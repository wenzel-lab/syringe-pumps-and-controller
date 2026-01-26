import threading

class SyringePumpController:
    def __init__(self, port, baudrate=115200, timeout=1, write_timeout=1):
        
        import serial, time
        
        self.ser = serial.Serial(port,
                                 baudrate,
                                 timeout=timeout,
                                 write_timeout=write_timeout,
                                 dsrdtr=True,
                                 rtscts=False)
        self._lock = threading.Lock()
        self.ser.reset_input_buffer()
        self.ser.rts = True
        time.sleep(1.0)
        self.ser.reset_input_buffer()  # discard boot-loader banner
        self.pumps = ['A', 'B', 'C', 'D']

    # ─────────────────────────────────────────────
    # High-level setters
    # ─────────────────────────────────────────────
    def set_flow      (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, FLOW=val))
    def set_diameter  (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, DIAMETER=val))
    def set_direction (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, DIRECTION=val))
    def set_state     (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, STATE=val))

    def set_unit      (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, UNIT=val))          # UL/MIN | UL/HR | ML/MIN | ML/HR
    def set_gearbox   (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, GEARBOX=val))      # "1:1" | "25:1" | "100:1"
    def set_microstep (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, MICROSTEP=val))     # "1/8" … "1/64"
    def set_threadrod (self, pump, val): return self._transaction(
                                             self._build_set_cmd(pump, ROD=val))           # "1-START" | "4-START"
    def set_enable    (self, pump, on ): return self._transaction(
                                             self._build_set_cmd(pump, ENABLE=("ON" if on else "OFF")))

    # ─────────────────────────────────────────────
    # GET helper
    # ─────────────────────────────────────────────
    def get_pump_status(self, pump):
        return self._transaction(f"GET PUMP={pump} STATUS")

    def _parse_status_response(self, response, param_name, default=None, value_type=str):
        """Helper to parse a parameter from the status response"""
        parts = response.split()
        for part in parts:
            if part.startswith(f"{param_name}="):
                value = part.split('=', 1)[1]
                try:
                    if value_type == float:
                        return float(value)
                    elif value_type == int:
                        return int(value)
                    elif value_type == bool:
                        return value.upper() == 'ON' or value.upper() == 'RUN'
                    return value
                except (ValueError, IndexError):
                    pass
        return default

    def get_flow(self, pump):
        """Get current flow rate for the specified pump"""
        response = self.get_pump_status(pump)
        return self._parse_status_response(response, 'FLOW', 0.0, float)

    def get_diameter(self, pump):
        """Get current diameter for the specified pump"""
        response = self.get_pump_status(pump)
        return self._parse_status_response(response, 'DIAMETER', 10.0, float)

    def get_direction(self, pump):
        """Get current direction for the specified pump (1 for infuse, -1 for withdraw)"""
        response = self.get_pump_status(pump)
        direction = self._parse_status_response(response, 'DIRECTION', 'INFUSE')
        if isinstance(direction, str):
            return 1 if direction.upper() == 'INFUSE' else -1
        return direction

    def get_state(self, pump):
        """Get current state (False for stopped, True for running)"""
        response = self.get_pump_status(pump)
        state = self._parse_status_response(response, 'STATE', 'STOP')
        return state.upper() == 'RUN'

    def get_unit(self, pump):
        """Get current unit setting"""
        response = self.get_pump_status(pump)
        return self._parse_status_response(response, 'UNIT', 'UL/HR')

    def get_gearbox(self, pump):
        """Get current gearbox setting"""
        response = self.get_pump_status(pump)
        return self._parse_status_response(response, 'GEARBOX', '1:1')

    def get_microstep(self, pump):
        """Get current microstep setting"""
        response = self.get_pump_status(pump)
        return self._parse_status_response(response, 'MICROSTEP', '1/16')

    def get_threadrod(self, pump):
        """Get current thread rod setting"""
        response = self.get_pump_status(pump)
        return self._parse_status_response(response, 'ROD', '1-START')

    def get_enable(self, pump):
        """Get current enable status"""
        response = self.get_pump_status(pump)
        return self._parse_status_response(response, 'ENABLE', 'OFF') == 'ON'
 
    # ─────────────────────────────────────────────
    # Internals
    # ─────────────────────────────────────────────
    def _send_set(self, pump, **kwargs):
        cmd = self._build_set_cmd(pump, **kwargs)
        self._write(cmd + '\n')
        return self._readline()

    def _build_set_cmd(self, pump, **kw):
        parts = [f"SET PUMP={pump}"]
        for k, v in kw.items():
            parts.append(f"{k}={v}")
        return ' '.join(parts)

    def _transaction(self, cmd: str) -> str:
        with self._lock:
            self._write(cmd + '\n')
            return self._readline()
    
    def _write(self, s): self.ser.write(s.encode())
    def _readline(self): return self.ser.readline().decode(errors='ignore').strip()
    def close(self): self.ser.close()