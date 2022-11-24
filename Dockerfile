FROM ubuntu:20.04
ARG SDK_VERSION=4.2.4
# https://docs.espressif.com/projects/esp-idf/en/v4.2/esp32/get-started/linux-setup.html
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y \
        git wget flex bison gperf python3 python3-pip python3-setuptools \
        cmake ninja-build ccache libffi-dev libssl-dev dfu-util \
        # Make Python 3 the default interpreter
        python-is-python3 \
    && rm -rf /var/lib/apt/lists/*
RUN useradd --groups dialout --create-home --shell /bin/bash user
RUN mkdir --parents /work/source && chown -R user:user /work
USER user
RUN mkdir -p ~/esp && cd ~/esp \
    && git clone --recursive --depth 1 \
        -b "v${SDK_VERSION}" https://github.com/espressif/esp-idf.git
RUN cd ~/esp/esp-idf && ./install.sh esp32
COPY docker-entrypoint.sh /home/user/
WORKDIR /work/source
ENTRYPOINT ["bash", "/home/user/docker-entrypoint.sh"]
