import json
import os
import subprocess
import unittest
from pathlib import Path

CLIENT_ROOT = Path(__file__).resolve().parents[1]

NODE_SCRIPT = r"""
import { requireLogin, requireRole } from './src/middleware/auth.js';
import process from 'node:process';

function simulateRequireLoginWithoutToken() {
  const req = { session: { roles: ['OPERATOR'] } };
  const result = { redirectCalled: false, redirectPath: null, nextCalled: false };
  const res = {
    redirect(path) {
      result.redirectCalled = true;
      result.redirectPath = path;
    }
  };
  requireLogin(req, res, () => {
    result.nextCalled = true;
  });
  return result;
}

function simulateRequireLoginWithToken() {
  const req = { session: { token: 'abc123', roles: ['OPERATOR'] } };
  const result = { redirectCalled: false, nextCalled: false };
  const res = {
    redirect() {
      result.redirectCalled = true;
    }
  };
  requireLogin(req, res, () => {
    result.nextCalled = true;
  });
  return result;
}

function simulateRequireRoleDenied() {
  const req = { session: { roles: ['OPERATOR'] } };
  const result = { statusCode: null, viewName: null, nextCalled: false };
  const res = {
    status(code) {
      result.statusCode = code;
      return this;
    },
    render(view) {
      result.viewName = view;
    }
  };
  requireRole('ADMIN')(req, res, () => {
    result.nextCalled = true;
  });
  return result;
}

function simulateRequireRoleAllowed() {
  const req = { session: { roles: ['ADMIN', 'OPERATOR'] } };
  const result = { nextCalled: false, statusInvoked: false, renderInvoked: false };
  const res = {
    status() {
      result.statusInvoked = true;
      return this;
    },
    render() {
      result.renderInvoked = true;
    }
  };
  requireRole('ADMIN')(req, res, () => {
    result.nextCalled = true;
  });
  return result;
}

function main() {
  const caseName = process.env.CASE_NAME;
  let result;

  switch (caseName) {
    case 'requireLoginWithoutToken':
      result = simulateRequireLoginWithoutToken();
      break;
    case 'requireLoginWithToken':
      result = simulateRequireLoginWithToken();
      break;
    case 'requireRoleDenied':
      result = simulateRequireRoleDenied();
      break;
    case 'requireRoleAllowed':
      result = simulateRequireRoleAllowed();
      break;
    default:
      throw new Error('Unknown case: ' + caseName);
  }

  process.stdout.write(JSON.stringify(result));
}

main();
"""


def _run_node_case(case_name: str) -> dict:
    env = os.environ.copy()
    env["CASE_NAME"] = case_name
    completed = subprocess.run(
        ["node", "--input-type=module", "-e", NODE_SCRIPT],
        check=True,
        capture_output=True,
        text=True,
        cwd=CLIENT_ROOT,
        env=env,
    )
    return json.loads(completed.stdout.strip())


class AuthMiddlewareTests(unittest.TestCase):
  """Pruebas unitarias sobre el middleware del cliente web."""

  def test_require_login_without_token_redirects(self):
    scenario = _run_node_case("requireLoginWithoutToken")
    self.assertTrue(scenario["redirectCalled"])
    self.assertEqual(scenario["redirectPath"], "/login")
    self.assertFalse(scenario["nextCalled"])
    print("Prueba 1 - login sin token redirige: OK")

  def test_require_login_with_token_calls_next(self):
    scenario = _run_node_case("requireLoginWithToken")
    self.assertFalse(scenario["redirectCalled"])
    self.assertTrue(scenario["nextCalled"])
    print("Prueba 2 - login con token continua: OK")

  def test_require_role_without_permission_returns_403(self):
    scenario = _run_node_case("requireRoleDenied")
    self.assertEqual(scenario["statusCode"], 403)
    self.assertEqual(scenario["viewName"], "403")
    self.assertFalse(scenario["nextCalled"])
    print("Prueba 3 - rol insuficiente devuelve 403: OK")

  def test_require_role_with_permission_calls_next(self):
    scenario = _run_node_case("requireRoleAllowed")
    self.assertTrue(scenario["nextCalled"])
    self.assertFalse(scenario["statusInvoked"])
    self.assertFalse(scenario["renderInvoked"])
    print("Prueba 4 - rol correcto permite continuar: OK")


if __name__ == "__main__":
    unittest.main()
