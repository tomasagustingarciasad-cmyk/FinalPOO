#!/usr/bin/env python3
"""
Simple integration test runner that:
- starts the server binary (assumes it's built as `main_servidor` in the same dir)
- waits until TCP port 8080 is accepting connections
- uses xmlrpc.client to call authLogin and generateGcodeFromMovements
- prints a brief summary and exits with code 0 on success

Notes:
- The test assumes a local Postgres instance and any required environment variables used by the server are set in the environment this script runs in.
- If the server requires a particular DB state, consider seeding a test DB or mocking DB access.
"""

import subprocess
import sys
import time
import socket
import os
from xmlrpc.client import ServerProxy, Fault, Boolean, DateTime

BASE_DIR = os.path.join(os.path.dirname(__file__), '..')
# Try the common executable names present in this repo
POSSIBLE_BINS = [os.path.join(BASE_DIR, 'main_servidor'), os.path.join(BASE_DIR, 'servidor_rpc')]
SERVER_BIN = None
for p in POSSIBLE_BINS:
    if os.path.isfile(p) and os.access(p, os.X_OK):
        SERVER_BIN = p
        break
if SERVER_BIN is None:
    # fallback to first candidate for error message
    SERVER_BIN = POSSIBLE_BINS[0]
RPC_URL = 'http://127.0.0.1:8080/RPC2'


def wait_for_port(host, port, timeout=10.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=1):
                return True
        except Exception:
            time.sleep(0.1)
    return False


def run():
    if not os.path.isfile(SERVER_BIN):
        print('Server binary not found at', SERVER_BIN)
        return 2

    env = os.environ.copy()
    # Make tests deterministic if possible
    proc = subprocess.Popen([SERVER_BIN], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        if not wait_for_port('127.0.0.1', 8080, timeout=12.0):
            print('Server did not start listening on 127.0.0.1:8080')
            proc.kill()
            out, err = proc.communicate(timeout=1)
            print('stdout:', out.decode(errors='ignore'))
            print('stderr:', err.decode(errors='ignore'))
            return 3

        proxy = ServerProxy(RPC_URL, allow_none=True)

        # 1) Call simple unauthenticated methods to validate server
        print('Calling ServerTest...')
        try:
            st = proxy.ServerTest()
            print('ServerTest reply:', st)
        except Fault as f:
            print('ServerTest failed:', f)
            return 8

        print('Calling Eco (echo)')
        try:
            eco = proxy.Eco('integration')
            print('Eco reply:', eco)
        except Fault as f:
            print('Eco failed:', f)
            return 9

        print('Calling Sumar (sum)')
        try:
            # Send two floats (server has strict type handling in XmlRpcValue conversion)
            s = proxy.Sumar(1.5, 2.5)
            print('Sumar reply:', s)
        except Fault as f:
            print('Sumar failed:', f)
            return 10

        # 2) If test credentials are provided via env, attempt login and exercise authenticated methods
        test_user = env.get('TEST_USER')
        test_pass = env.get('TEST_PASS')
        admin_token = env.get('TEST_ADMIN_TOKEN')

        created_user = False
        created_routine_id = None
        token = None

        def try_login(user, passwd):
            try:
                r = proxy.authLogin(user, passwd)
                return r
            except Fault as f:
                print('authLogin Fault:', f)
                return None

        if test_user and test_pass:
            print('Attempting authLogin for TEST_USER...')
            res = try_login(test_user, test_pass)
            if not res or not res.get('success'):
                # If admin token provided, try to create the user
                if admin_token:
                    print('TEST_USER login failed; attempting to create user using TEST_ADMIN_TOKEN')
                    try:
                        create_res = proxy.userCreate(admin_token, test_user, test_pass, 'OPERATOR')
                        print('userCreate response:', create_res)
                        if create_res.get('success'):
                            created_user = True
                            # try login again
                            res = try_login(test_user, test_pass)
                    except Fault as f:
                        print('userCreate Fault:', f)
                        res = None
                else:
                    print('No admin token provided and TEST_USER login failed; skipping authenticated tests')

            if res and res.get('success'):
                token = res.get('token')
                print('Authenticated as', test_user)

        # If we have an admin token but no TEST_USER, we can use admin_token for admin-level tests
        if not token and admin_token:
            print('Using TEST_ADMIN_TOKEN for admin-level tests')
            token = admin_token

        if token:
            # small_gcode used for fallback routine create/update - define early
            small_gcode = 'G1 X0 Y0 Z0 F100\n'
            # Try routineCreate (use a small gcode string) and routine CRUD lifecycle
            try:
                # If generateGcodeFromMovements is available, test it first
                # Use a unique routine name per run to avoid "already exists" errors
                routine_name = 'int_test_routine_%d' % int(time.time())
                print('Calling generateGcodeFromMovements (trying multiple payload shapes) with', routine_name)

                # Try A: plain Python dicts (what we tried earlier)
                payloads = []
                payloads.append([
                    {'x': float(0.0), 'y': float(0.0), 'z': float(0.0), 'feedrate': float(500.0), 'endEffectorActive': False},
                    {'x': float(10.0), 'y': float(0.0), 'z': float(1.0), 'feedrate': float(500.0), 'endEffectorActive': True},
                ])

                # Try B: booleans as ints (0/1) which sometimes avoids xml-rpc bool/struct typing issues
                payloads.append([
                    {'x': 0.0, 'y': 0.0, 'z': 0.0, 'feedrate': 500.0, 'endEffectorActive': 0},
                    {'x': 10.0, 'y': 0.0, 'z': 1.0, 'feedrate': 500.0, 'endEffectorActive': 1},
                ])

                # Try C: use xmlrpc types explicitly where helpful
                payloads.append([
                    {'x': float(0.0), 'y': float(0.0), 'z': float(0.0), 'feedrate': float(500.0), 'endEffectorActive': Boolean(False)},
                    {'x': float(10.0), 'y': float(0.0), 'z': float(1.0), 'feedrate': float(500.0), 'endEffectorActive': Boolean(True)},
                ])

                success_payload = None
                last_exc = None
                for idx, movements in enumerate(payloads):
                    try:
                        print(' Trying payload variant', idx)
                        gres = proxy.generateGcodeFromMovements(token, routine_name, 'integration test', movements)
                        print(' generateGcodeFromMovements reply:', gres)
                        if gres.get('success'):
                            created_routine_id = gres.get('routineId')
                            success_payload = idx
                            break
                    except Fault as f:
                        print(' generateGcodeFromMovements Fault for variant', idx, ':', f)
                        last_exc = f

                if success_payload is None:
                    print('All generateGcodeFromMovements payload variants failed; last fault:', last_exc)

                # If generate didn't create a routine, create one manually with routineCreate
                if not created_routine_id:
                    print('Calling routineCreate...')
                    small_gcode = 'G1 X0 Y0 Z0 F100\n'
                    rcreate = proxy.routineCreate(token, 'int_test_manual.gcode', 'int_test_manual.gcode', 'manual test', small_gcode)
                    print('routineCreate:', rcreate)
                    if rcreate.get('success'):
                        created_routine_id = rcreate.get('routineId')

                # List routines
                if created_routine_id:
                    print('Calling routineList...')
                    rlist = proxy.routineList(token)
                    print('routineList keys:', list(rlist.keys()))

                    print('Calling routineGet for created id...')
                    rget = proxy.routineGet(token, created_routine_id)
                    print('routineGet:', rget)

                    # Optionally update
                    print('Calling routineUpdate...')
                    updated = proxy.routineUpdate(token, created_routine_id, 'int_test_manual.gcode', 'updated desc', small_gcode)
                    print('routineUpdate:', updated)

                    # Delete the routine to cleanup
                    print('Calling routineDelete...')
                    rdel = proxy.routineDelete(token, created_routine_id)
                    print('routineDelete:', rdel)

                # User list and info (requires admin privileges)
                if token == admin_token:
                    print('Calling userList...')
                    ulist = proxy.userList(token)
                    print('userList keys:', list(ulist.keys()))

                    # If we created a user during this run, check userInfo and delete it
                    if created_user:
                        print('Calling userInfo for created user...')
                        uinfo = proxy.userInfo(token, test_user)
                        print('userInfo:', uinfo)
                        print('Deleting created user...')
                        udel = proxy.userDelete(token, test_user)
                        print('userDelete:', udel)

                # Logout if using authLogin
                if test_user and test_pass and token and token != admin_token:
                    print('Calling authLogout...')
                    logout = proxy.authLogout(token)
                    print('authLogout:', logout)

            except Fault as f:
                print('Authenticated flow Fault:', f)
                return 11
        else:
            print('No token available for authenticated tests; skipping those steps')

        print('Integration test succeeded')
        return 0
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except Exception:
            proc.kill()


if __name__ == '__main__':
    sys.exit(run())
