FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

WORKDIR /rules_service

COPY ./rules_service/ .

COPY ./shared_data_model /shared_data_model

RUN pip3 install kafka-python protobuf && cd /tmp && git clone https://github.com/dmlc/xgboost.git \
    && cd xgboost && git checkout v3.1.1 \
    && git submodule update --init --recursive && mkdir -p build \
    && cd build && cmake -DUSE_CUDA=OFF -DUSE_OPENMP=ON .. && make -j$(nproc) && sudo make install

RUN apt-get update && \
    apt-get install -y python3-pip && \
    pip3 install lightgbm && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN . ./.env && \
    cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build ./build


CMD ["/bin/bash", "-c", ". ./.env && ./build/src/bin/rules_service --config ./configs/static_config.yaml"]