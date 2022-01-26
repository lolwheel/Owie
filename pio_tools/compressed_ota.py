# Based on pfeerick/compressed_ota.py
import gzip
import shutil
import os
Import("env")


def compressFirmware(source, target, env):
    """ Compress ESP8266 firmware using gzip for 'compressed OTA upload' """
    SOURCE_FILE = env.subst("$BUILD_DIR") + os.sep + env.subst("$PROGNAME") + ".bin"
    UNCOMPRESSED_FILE = SOURCE_FILE+'.uncompressed'
    print("Compressing firmware for upload...")
    shutil.move(SOURCE_FILE, UNCOMPRESSED_FILE)
    with open(UNCOMPRESSED_FILE, 'rb') as f_in:
        with gzip.open(SOURCE_FILE, 'wb') as f_out:
            shutil.copyfileobj(f_in, f_out)

    ORG_FIRMWARE_SIZE = os.stat(UNCOMPRESSED_FILE).st_size
    GZ_FIRMWARE_SIZE = os.stat(SOURCE_FILE).st_size

    print("Compression reduced firmware size to {:.0f}% of original (was {} bytes, now {} bytes)".format((GZ_FIRMWARE_SIZE / ORG_FIRMWARE_SIZE) * 100, ORG_FIRMWARE_SIZE, GZ_FIRMWARE_SIZE))

env.AddPreAction("upload", compressFirmware)