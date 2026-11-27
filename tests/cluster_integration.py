#!/usr/bin/env python3
"""Real isolated MySQL + Redis + two ChatServers + Nginx; no existing DB is modified."""
import json
import os
import re
from pathlib import Path
import shutil
import signal
import socket
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def free_port():
    with socket.socket() as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]


def eventually(check, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if check():
            return
        time.sleep(.1)
    raise AssertionError('Condition timed out')


def listening(port):
    try:
        with socket.create_connection(('127.0.0.1', port), timeout=.2):
            return True
    except OSError:
        return False


class Client:
    def __init__(self, port):
        self.sock = socket.create_connection(('127.0.0.1', port), timeout=5)
        self.local_port = self.sock.getsockname()[1]
        self.reader = self.sock.makefile('rb')

    def send(self, obj, delimiter=b'\n'):
        self.sock.sendall(json.dumps(obj, ensure_ascii=False).encode() + delimiter)

    def receive(self):
        line = self.reader.readline()
        assert line, 'Connection closed unexpectedly'
        return json.loads(line)

    def register(self, name):
        self.send({'msgid': 4, 'name': name, 'password': 'test-password'})
        reply = self.receive()
        assert reply['error'] == 0, reply
        return reply['id']

    def login(self, userid):
        self.send({'msgid': 1, 'id': userid, 'password': 'test-password'})
        return self.receive()

    def close(self):
        self.reader.close()
        self.sock.close()


def main():
    for executable in ('mysqld', 'mysql', 'redis-server', 'redis-cli', 'nginx'):
        assert shutil.which(executable), f'Missing {executable}'
    assert os.geteuid() == 0, 'Run this isolated MySQL test via sudo python3 tests/cluster_integration.py'
    processes, clients, handles = [], [], []
    ports = set()
    while len(ports) < 5:
        ports.add(free_port())
    mysql_port, redis_port, port_a, port_b, proxy_port = ports
    with tempfile.TemporaryDirectory(prefix='chatserver-cluster-') as temp:
        temp = Path(temp)

        def spawn(args, name, env=None):
            handle = open(temp / (name + '.log'), 'w')
            handles.append(handle)
            process = subprocess.Popen(args, cwd=ROOT, env=env,
                                       stdout=handle, stderr=subprocess.STDOUT)
            processes.append(process)
            return process

        def sql(statement):
            return subprocess.check_output([
                'mysql', '--protocol=TCP', '-h127.0.0.1', '-uroot',
                f'-P{mysql_port}', '--batch', '--skip-column-names',
                '-e', statement], text=True).strip()

        def redis(*args):
            return subprocess.check_output(['redis-cli', '-p', str(redis_port), '--raw', *args], text=True).strip()

        def client(port):
            value = Client(port)
            clients.append(value)
            return value

        try:
            data = temp / 'mysql'
            data.mkdir()
            subprocess.run(['mysqld', '--no-defaults', '--initialize-insecure',
                            '--user=root', f'--datadir={data}',
                            f'--log-error={temp / "mysql-init.log"}'], check=True, timeout=60)
            spawn(['mysqld', '--no-defaults', '--user=root', f'--datadir={data}',
                   f'--port={mysql_port}', '--bind-address=127.0.0.1',
                   f'--socket={temp / "mysql.sock"}', f'--pid-file={temp / "mysql.pid"}',
                   '--mysqlx=OFF', '--innodb-use-native-aio=OFF'], 'mysql')
            eventually(lambda: listening(mysql_port), 45)
            sql('CREATE DATABASE chat_cluster_test CHARACTER SET utf8mb4;')
            schema = (ROOT / 'deploy/schema.sql').read_text()
            sql('USE chat_cluster_test;\n' + schema)
            redis_args = ['redis-server', '--bind', '127.0.0.1', '--port', str(redis_port),
                          '--save', '', '--appendonly', 'no']
            redis_process = spawn(redis_args, 'redis')
            eventually(lambda: listening(redis_port))
            env = dict(os.environ, CHAT_DB_HOST='127.0.0.1', CHAT_DB_PORT=str(mysql_port),
                       CHAT_DB_USER='root', CHAT_DB_PASSWORD='', CHAT_DB_NAME='chat_cluster_test',
                       CHAT_REDIS_HOST='127.0.0.1', CHAT_REDIS_PORT=str(redis_port),
                       CHAT_REDIS_PASSWORD='', CHAT_REDIS_PREFIX='integration:user:')
            binary = str(ROOT / 'chatserver/bin/ChatServer')
            server_a = spawn([binary, '127.0.0.1', str(port_a)], 'server-a', env)
            spawn([binary, '127.0.0.1', str(port_b)], 'server-b', env)
            eventually(lambda: listening(port_a) and listening(port_b))
            config = (ROOT / 'deploy/nginx.conf').read_text()
            config = config.replace(':6001', f':{port_a}').replace(':6002', f':{port_b}')
            config = config.replace(':8000', f':{proxy_port}')
            nginx_config = temp / 'nginx.conf'
            nginx_config.write_text(config)
            subprocess.run(['nginx', '-t', '-p', str(temp) + '/', '-c', str(nginx_config)], check=True)
            spawn(['nginx', '-p', str(temp) + '/', '-c', str(nginx_config), '-g', 'daemon off;'], 'nginx')
            eventually(lambda: listening(proxy_port))
            # Probe sessions are short lived; let least_conn counters settle.
            time.sleep(.3)

            alice, bob = client(proxy_port), client(proxy_port)
            aid, bid = alice.register('alice'), bob.register('bob')
            assert alice.login(aid)['error'] == 0
            assert bob.login(bid)['error'] == 0
            assert redis('PUBSUB', 'NUMSUB', f'integration:user:{bid}').splitlines()[-1] == '1'
            alice.send({'msgid': 6, 'id': aid, 'toid': bid, 'msg': 'cross-server hello'})
            assert bob.receive()['msg'] == 'cross-server hello'
            bob.send({'msgid': 6, 'id': bid, 'toid': aid, 'msg': 'reply'})
            assert alice.receive()['msg'] == 'reply'
            print('PASS: Nginx connections exchange messages through Redis', flush=True)

            other = client(port_b)
            assert other.login(aid)['error'] == 2
            sql(f"INSERT INTO chat_cluster_test.all_group VALUES(1,'group','test');"
                f"INSERT INTO chat_cluster_test.groupuser VALUES({aid},1,'creator'),({bid},1,'normal');")
            alice.send({'msgid': 10, 'id': aid, 'groupid': 1, 'msg': 'group hello'})
            assert bob.receive()['msg'] == 'group hello'
            print('PASS: duplicate-login rejection and cross-server group chat', flush=True)

            offline = client(port_b)
            oid = offline.register('offline')
            text = "离线消息 with apostrophe ' " + 'x' * 5000
            alice.send({'msgid': 6, 'id': aid, 'toid': oid, 'msg': text})
            eventually(lambda: sql(f'SELECT COUNT(*) FROM chat_cluster_test.offlinemsg WHERE userid={oid}') == '1')
            reply = offline.login(oid)
            assert reply['error'] == 0
            assert json.loads(reply['offlinemsg'][0])['msg'] == text
            print('PASS: offline fallback and large UTF-8 login response', flush=True)

            native = str(ROOT / 'chatserver/bin/ChatClient')
            result = subprocess.run([native, '127.0.0.1', str(proxy_port)],
                                    input='2\nnative-client\ntest-password\n3\n',
                                    text=True, capture_output=True, timeout=10)
            assert result.returncode == 0 and 'reg success' in result.stdout, result
            nid = int(sql("SELECT id FROM chat_cluster_test.user WHERE name='native-client'"))
            alice.send({'msgid': 6, 'toid': nid, 'msg': text, 'time': 'test-time', 'name': 'alice'})
            eventually(lambda: sql(f'SELECT COUNT(*) FROM chat_cluster_test.offlinemsg WHERE userid={nid}') == '1')
            result = subprocess.run([native, '127.0.0.1', str(proxy_port)],
                                    input=f'1\n{nid}\ntest-password\nloginout\n',
                                    text=True, capture_output=True, timeout=10)
            assert result.returncode == 0 and 'test-time' in result.stdout and 'bye' in result.stdout, result
            print('PASS: compiled C++ client registers, receives large login reply, and logs out through Nginx', flush=True)

            # Fragmented requests and coalesced requests use the same framed protocol.
            packet = json.dumps({'msgid': 6, 'toid': bid, 'msg': 'fragmented'}).encode() + b'\n'
            alice.sock.sendall(packet[:7])
            time.sleep(.05)
            alice.sock.sendall(packet[7:])
            assert bob.receive()['msg'] == 'fragmented'
            packet1 = json.dumps({'msgid': 6, 'toid': bid, 'msg': 'one'}).encode() + b'\0'
            packet2 = json.dumps({'msgid': 6, 'toid': bid, 'msg': 'two'}).encode() + b'\n'
            alice.sock.sendall(packet1 + packet2)
            assert [bob.receive()['msg'], bob.receive()['msg']] == ['one', 'two']
            print('PASS: TCP fragmentation, coalescing, and legacy NUL delimiter', flush=True)

            bob.send({'msgid': 3, 'id': aid})  # Cannot log Alice out by forging her ID.
            eventually(lambda: sql(f'SELECT state FROM chat_cluster_test.user WHERE id={bid}') == 'offline')
            assert sql(f'SELECT state FROM chat_cluster_test.user WHERE id={aid}') == 'online'
            assert redis('PUBSUB', 'NUMSUB', f'integration:user:{bid}').splitlines()[-1] == '0'
            assert bob.login(bid)['error'] == 0
            print('PASS: owner-only logout, unsubscribe, and resubscribe', flush=True)

            redis_process.terminate()
            redis_process.wait(timeout=10)
            redis_process = spawn(redis_args, 'redis-restarted')
            eventually(lambda: listening(redis_port))
            eventually(lambda: redis('PUBSUB', 'NUMSUB', f'integration:user:{bid}').splitlines()[-1] == '1')
            # First publish on a dead publisher connection may fall back offline; do not retry it blindly.
            alice.send({'msgid': 6, 'toid': bid, 'msg': 'publisher-reconnect'})
            time.sleep(.3)
            alice.send({'msgid': 6, 'toid': bid, 'msg': 'after-reconnect'})
            seen = bob.receive()['msg']
            if seen == 'publisher-reconnect':
                seen = bob.receive()['msg']
            assert seen == 'after-reconnect'
            print('PASS: Redis reconnect and automatic subscription recovery', flush=True)

            # A direct user on B must survive graceful shutdown of A.
            survivor = client(port_b)
            sid = survivor.register('survivor')
            assert survivor.login(sid)['error'] == 0
            server_a.send_signal(signal.SIGTERM)
            server_a.wait(timeout=15)
            assert server_a.returncode == 0
            assert sql(f'SELECT state FROM chat_cluster_test.user WHERE id={sid}') == 'online'
            survivor.close()
            eventually(lambda: sql(f'SELECT state FROM chat_cluster_test.user WHERE id={sid}') == 'offline')
            print('PASS: process-local graceful shutdown and disconnect cleanup', flush=True)

            for value in clients:
                value.close()
            time.sleep(.3)
            log = (temp / 'access.log').read_text()
            assert f'127.0.0.1:{port_a}' in log and f'127.0.0.1:{port_b}' in log, log
            alice_route = re.search(rf'127\.0\.0\.1:{alice.local_port} -> 127\.0\.0\.1:(\d+)', log)
            bob_route = re.search(rf'127\.0\.0\.1:{bob.local_port} -> 127\.0\.0\.1:(\d+)', log)
            assert alice_route and bob_route and alice_route[1] != bob_route[1], log
            print('PASS: Nginx access log confirms both upstream servers were used', flush=True)
        except BaseException:
            for path in temp.glob('*.log'):
                print(f'--- {path.name} ---\n{path.read_text(errors="replace")[-6000:]}', flush=True)
            raise
        finally:
            for value in clients:
                value.close()
            for process in reversed(processes):
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait()
            for handle in handles:
                handle.close()


if __name__ == '__main__':
    main()
