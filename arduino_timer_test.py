#!/usr/bin/python3

import serial
import unittest
import time

PORT = "/dev/ttyACM0"
BAUDRATE=76800
_VERBOSE=False

class ArduinoTimerTestCase(unittest.TestCase):
  def setUp(self):
    self.serial = serial.Serial(port=PORT, baudrate=BAUDRATE, timeout=2)
    self.assertTrue(self.serial.is_open)

    attempts = 0
    while True:
      self.writeCommand("h")
      response = self.readResponse() 
      if response == "Hello":
        break
      attempts = attempts + 1
      assert(attempts < 10)

  def tearDown(self):
    self.serial.flushOutput()
    self.serial.close()
    self.serial = None

  def writeCommand(self, cmd):
    if (_VERBOSE):
      print('cmd = %s' % cmd)
    bytes = self.serial.write(cmd.encode("ascii"))
    self.assertEqual(bytes, len(cmd),
                     'failed to write string %s over serial connection' % str)

  def readResponse(self):
    response = self.serial.readline().decode("ascii").rstrip('\r\n');
    if (_VERBOSE):
      print('response = %s' % response)
    return response

  # Test helpers
  def reset_timers(self):
    self.writeCommand("r")
    response = self.readResponse()
    self.assertEqual(response, "Rok")

  def clear_timers(self):
    self.writeCommand("c")
    response = self.readResponse()
    self.assertEqual(response, "Cok")

  def set_timer_enables(self, enables):
    self.writeCommand("f" + enables)
    response = self.readResponse()
    self.assertEqual(response, "Fok")

  def set_timer_thresholds(self, thresholds):
    self.writeCommand("u" + thresholds)
    response = self.readResponse()
    self.assertEqual(response, "Uok")

  def start_timers(self):
    self.writeCommand("g")
    response = self.readResponse()
    self.assertEqual(response, "Gok")

  def stop_timers(self):
    self.writeCommand("s")
    response = self.readResponse()
    self.assertEqual(response, "Sok")

  def test_reset(self):
    self.reset_timers()

  def test_enables(self):
    self.reset_timers()
    self.set_timer_enables("0101")

    self.writeCommand("e")
    response = self.readResponse()
    self.assertEqual(response, "E0101")

    self.set_timer_enables("1010")
    self.writeCommand("e")
    response = self.readResponse()
    self.assertEqual(response, "E1010")
    
  def test_infos(self):
    self.writeCommand("i")
    response = self.readResponse()
    self.assertTrue(response.startswith('I'))

  def test_threshold(self):
    self.reset_timers()

    self.set_timer_thresholds("1,2,3,4")
    self.writeCommand("t")
    response = self.readResponse()
    self.assertEqual(response, "T1,2,3,4")

    self.set_timer_thresholds("0,0,0,0")
    self.writeCommand("t")
    response = self.readResponse()
    self.assertEqual(response, "T0,0,0,0")
    
  def test_analogue_read(self):
    self.writeCommand("a")
    response = self.readResponse()
    self.assertTrue(response.startswith("A"))
    self.assertFalse(response.startswith("Afailed:"))
    values = response[1:].split(",")
    self.assertEqual(len(values), 4)
    self.assertTrue(all([s.isdigit() for s in values]))

  def test_go_latency(self):
    self.reset_timers()
    self.set_timer_enables("1111")
  
    total_latency_ms = 0
    max_latency_ms = 0
    num_iterations = 100
    for i in range(num_iterations):
      start_time = time.perf_counter()
      self.writeCommand("g")
      response = self.readResponse()
      end_time = time.perf_counter()
      latency_ms = (end_time - start_time) * 1000
      self.assertEqual(response, "Gok")
      total_latency_ms += latency_ms
      max_latency_ms = max(max_latency_ms, latency_ms)

      self.stop_timers()
      self.clear_timers()

    average_latency_ms = total_latency_ms / num_iterations;
    self.assertLess(average_latency_ms, 5);
    self.assertLess(max_latency_ms, 5);

  def test_get_timer_values(self):
    self.reset_timers()
    self.set_timer_enables("1010")

    self.writeCommand("v")
    response = self.readResponse()
    self.assertEqual(response,"V0,0,0,0")
  
    # run timer for a bit.
    self.start_timers()
    sleep_duration_ms = 100
    time.sleep(sleep_duration_ms * 0.001)
    self.stop_timers()
  
    self.writeCommand("v")
    response = self.readResponse()
    values = [int(x) for x in response[1:].split(",")]
    self.assertEqual(len(values), 4)
    self.assertGreater(values[0], sleep_duration_ms)
    self.assertEqual(values[1], 0)
    self.assertGreater(values[2], sleep_duration_ms)
    self.assertEqual(values[3], 0)
  
  def test_get_status(self):
    self.reset_timers()
    self.writeCommand("q")
    response = self.readResponse()
    self.assertEqual(response,"QDDDD")

    self.set_timer_enables("1100")
    self.writeCommand("q")
    response = self.readResponse()
    self.assertEqual(response,"QCCDD")
    
    self.set_timer_thresholds("0,32767,0,0")
    self.start_timers()
    time.sleep(0.1)
    
    self.writeCommand("q")
    response = self.readResponse()
    self.assertEqual(response,"QGTDD")

    self.stop_timers()
    self.writeCommand("q")
    response = self.readResponse()
    self.assertEqual(response,"QSTDD")

if __name__ == '__main__':
    unittest.main()
