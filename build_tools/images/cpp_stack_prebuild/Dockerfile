FROM gcc:15.2.0-bookworm

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    software-properties-common \
    git \
    wget \
    gnupg \
    ca-certificates \
    python3 \
    python3-pip \
    pipx \
    protobuf-compiler \
    tar && \
    rm -rf /var/lib/apt/lists/*


RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc | gpg --dearmor -o /usr/share/keyrings/kitware-archive-keyring.gpg && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    apt-get update && \
    apt-get install -y -o Dpkg::Options::="--force-overwrite" cmake && \
    rm -rf /var/lib/apt/lists/*

COPY ./cmakepresets /opt/cmakepresets

RUN PIPX_BIN_DIR=/usr/local/bin pipx install conan --pip-args="--no-cache-dir" && \
    conan profile detect

RUN printf '\n[conf]\ntools.cmake.cmaketoolchain:user_toolchain=["/opt/cmakepresets/toolchain-debug.cmake"]' >> "/root/.conan2/profiles/default"

COPY ./preinstall_conan /opt/preinstall_conan

RUN conan install /opt/preinstall_conan/protobuf --build=missing

RUN apt-get update && apt-get install -y \
    protobuf-compiler \
    libprotobuf-dev \
    && rm -rf /var/lib/apt/lists/*

COPY start.sh /start.sh
RUN chmod +x /start.sh

# CMD ["bash", "/start.sh"]
