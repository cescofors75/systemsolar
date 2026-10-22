"""Run esptool through the active Python interpreter on Windows."""
import sys

Import("env")

if sys.platform == "win32":
    command = '"{}" -m esptool'.format(sys.executable)
    env.Replace(OBJCOPY=command, ERASETOOL=command, UPLOADER=command)
