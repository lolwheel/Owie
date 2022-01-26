# pfeerick/compressed_ota.py
import gzip
import shutil
import os
Import("env")


def compressFirmware(source, target, env):
    """ Compress ESP8266 firmware using gzip for 'compressed OTA upload' """
    SOURCE_FILE = env.subst("$BUILD_DIR") + os.sep + env.subst("$PROGNAME") + ".bin"
    if not os.path.exists(SOURCE_FILE+'.bak'):
        print("Compressing firmware for upload...")
        shutil.move(SOURCE_FILE, SOURCE_FILE + '.bak')
        with open(SOURCE_FILE + '.bak', 'rb') as f_in:
            with gzip.open(SOURCE_FILE, 'wb') as f_out:
                shutil.copyfileobj(f_in, f_out)

    if os.path.exists(SOURCE_FILE+'.bak'):
        ORG_FIRMWARE_SIZE = os.stat(SOURCE_FILE + '.bak').st_size
        GZ_FIRMWARE_SIZE = os.stat(SOURCE_FILE).st_size

        print("Compression reduced firmware size to {:.0f}% of original (was {} bytes, now {} bytes)".format((GZ_FIRMWARE_SIZE / ORG_FIRMWARE_SIZE) * 100, ORG_FIRMWARE_SIZE, GZ_FIRMWARE_SIZE))

env.AddPreAction("upload", compressFirmware)