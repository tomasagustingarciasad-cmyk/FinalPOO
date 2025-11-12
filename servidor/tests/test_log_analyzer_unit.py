import io
import sys
import tempfile
import textwrap
import unittest
from contextlib import redirect_stdout
from datetime import datetime
from pathlib import Path

ROOT_DIR = Path(__file__).resolve().parents[2]
if str(ROOT_DIR) not in sys.path:
    sys.path.append(str(ROOT_DIR))

from servidor.log_analyzer import LogAnalyzer  # type: ignore  # pylint: disable=wrong-import-position


class LogAnalyzerUnitTests(unittest.TestCase):
    """Pruebas unitarias para la lógica de LogAnalyzer."""

    def setUp(self):
        self._tmp_dir = tempfile.TemporaryDirectory()
        self.csv_path = Path(self._tmp_dir.name) / "sample_log.csv"
        csv_content = textwrap.dedent(
            """
            timestamp,type,module,level,method,user,client_ip,response_code,message,details
            2025-05-10 10:00:00.000,REQUEST,auth,INFO,authLogin,alice,127.0.0.1,200,Inicio de sesion,
            2025-05-10 10:05:00.000,REQUEST,auth,ERROR,authLogin,alice,127.0.0.1,401,Credenciales rechazadas,
            2025-05-10 11:00:00.000,SYSTEM,robot,WARNING,,system,127.0.0.1,0,Robot desconectado,
            2025-05-10 11:30:00.000,REQUEST,routines,INFO,routineCreate,bob,192.168.1.10,201,Rutina creada,
            """
        ).strip()
        self.csv_path.write_text(csv_content, encoding="utf-8")

        buffer = io.StringIO()
        with redirect_stdout(buffer):
            self.analyzer = LogAnalyzer(str(self.csv_path))

    def tearDown(self):
        self._tmp_dir.cleanup()

    def test_entries_are_loaded(self):
        self.assertEqual(len(self.analyzer.entries), 4)
    print("Prueba 1 - entradas cargadas: OK")

    def test_filter_by_user_returns_only_matching_entries(self):
        entries = self.analyzer.filter_by_user("alice")
        self.assertEqual(len(entries), 2)
        self.assertTrue(all(entry.user == "alice" for entry in entries))
    print("Prueba 2 - filtro por usuario correcto: OK")

    def test_filter_by_level_is_case_insensitive(self):
        entries = self.analyzer.filter_by_level("error")
        self.assertEqual(len(entries), 1)
        self.assertEqual(entries[0].method, "authLogin")
    print("Prueba 3 - filtro por nivel sin distinguir mayusculas: OK")

    def test_filter_by_type_counts_requests(self):
        entries = self.analyzer.filter_by_type("request")
        self.assertEqual(len(entries), 3)
    print("Prueba 4 - conteo de solicitudes por tipo: OK")

    def test_filter_by_method_returns_expected_calls(self):
        entries = self.analyzer.filter_by_method("authLogin")
        self.assertEqual(len(entries), 2)
    print("Prueba 5 - filtro por metodo esperado: OK")

    def test_filter_by_date_range_limits_results(self):
        start = datetime.strptime("2025-05-10 10:00:00", "%Y-%m-%d %H:%M:%S")
        end = datetime.strptime("2025-05-10 10:10:00", "%Y-%m-%d %H:%M:%S")
        entries = self.analyzer.filter_by_date_range(start, end)
        self.assertEqual(len(entries), 2)
    print("Prueba 6 - filtro por rango de fechas: OK")

    def test_generate_summary_report_returns_string(self):
        report = self.analyzer.generate_summary_report()
        self.assertIsInstance(report, str)
        self.assertTrue(len(report) > 0)
    print("Prueba 7 - reporte resumen generado: OK")


if __name__ == "__main__":
    unittest.main()
