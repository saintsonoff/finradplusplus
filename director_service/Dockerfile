FROM ghcr.io/userver-framework/ubuntu-22.04-userver:latest

WORKDIR /director_service

COPY ./director_service/ .

COPY ./shared_data_model /shared_data_model

ENV DIRECTOR_SHARED_MODELS_PATH=/shared_data_model

RUN cmake -S ./src -B ./build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build ./build

RUN echo '#!/bin/bash\n\
set -e\n\
cd "$(dirname "$0")"\n\
\n\
export SHARED_MODELS_PATH="${DIRECTOR_SHARED_MODELS_PATH:-/shared_data_model}"\n\
\n\
CONFIG_VARS=""\n\
\n\
if [ -f "config_vars.yaml" ]; then\n\
    CONFIG_VARS="$(pwd)/config_vars.yaml"\n\
elif [ -f "configs/config_vars.yaml" ]; then\n\
    CONFIG_VARS="$(pwd)/configs/config_vars.yaml"\n\
else\n\
    echo "Warning: config_vars.yaml not found, using config_vars_template.yaml"\n\
    CONFIG_VARS="$(pwd)/configs/config_vars_template.yaml"\n\
fi\n\
\n\
echo "Using config vars: $CONFIG_VARS"\n\
\n\
exec ./build/bin/director_service \\\n\
    --config ./configs/static_config.yaml \\\n\
    --config_vars "$CONFIG_VARS" 2>&1\n\
' > run.sh && chmod +x run.sh

CMD ["bash", "-c", "./run.sh"]