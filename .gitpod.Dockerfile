FROM gitpod/workspace-full

USER gitpod

RUN pip3 install -U platformio && npm install html-minifier-terser -g