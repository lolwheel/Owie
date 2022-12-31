# Allows PlatformIO to upload directly to AsyncElegantOTA
#
# To use:
# - copy this script into the same folder as your platformio.ini
# - set the following for your project in platformio.ini:
#
# extra_scripts = platformio_upload.py
# upload_protocol = custom
# upload_url = <your upload URL>
# 
# An example of an upload URL:
# upload_URL = http://192.168.1.123/update

import requests
import hashlib
Import('env')

try:
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor
except ImportError:
    env.Execute("$PYTHONEXE -m pip install requests_toolbelt")
    from requests_toolbelt import MultipartEncoder, MultipartEncoderMonitor

def on_upload(source, target, env):
    firmware_path = str(source[0])
    upload_url = env.GetProjectOption('upload_url')

    with open(firmware_path, 'rb') as firmware:
        md5 = hashlib.md5(firmware.read()).hexdigest()
        firmware.seek(0)
        encoder = MultipartEncoder(fields={
            'MD5': md5, 
            'firmware': ('firmware', firmware, 'application/octet-stream')}
        )
        try:
            response = requests.post(upload_url, data=encoder, headers={'Content-Type': encoder.content_type})
            response.raise_for_status()
        except Exception as e:
            raise SystemExit(e)
        
        print('OTA finished successfully')
            
env.Replace(UPLOADCMD=on_upload)