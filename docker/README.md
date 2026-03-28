# nndeploy Docker Image (Ubuntu 22.04)

This Docker image builds and runs the [nndeploy](https://github.com/nndeploy/nndeploy) framework in a clean Ubuntu 22.04 environment. It includes all dependencies such as OpenCV, ONNX Runtime, and builds both the C++ backend and Python package.

## Available Docker Images

We currently support the following Docker configurations:

- **Dockerfile**: Basic version with ONNX Runtime only
- **Dockerfile.ort_ov**: Includes ONNX Runtime, OpenVINO
- **Dockerfile.ort_ov_trt**: Includes ONNX Runtime, OpenVINO, TensorRT
- **Dockerfile.ort_ov_mnn_trt**: Includes ONNX Runtime, OpenVINO, TensorRT, MNN
<!-- - **Dockerfile.ort_ascend**: Includes ONNX Runtime and Huawei Ascend inference engine
- **Dockerfile.ort_rknn**: Includes ONNX Runtime and Rockchip RKNN inference engine -->

### Development Environment

- **Dockerfile.ort_ov_trt_mnn.develop**: Development environment with local source code mount (ORT, OV, TRT, MNN)

This development image differs from production images by using local source code mounting instead of git cloning, making it ideal for development and debugging.

The following instructions use `Dockerfile` as an example, but the same operations apply to all other Docker files.

## Build the Image

```bash
# Use cache
docker build -f docker/Dockerfile -t nndeploy-linux .

# No cache
docker build --no-cache -f docker/Dockerfile -t nndeploy-linux .
````

## Run the Container (Default Port 8888)

```bash
docker run -it -p 8888:8888 nndeploy-linux
```

This will run:

```bash
python3 app.py --port 8888
```

## Run with Custom Port

```bash
docker run -it -p 9000:9000 nndeploy-linux python3 app.py --port 9000
```

## Run with Shell

```bash
docker run -it nndeploy-linux bash
```

## Save and Share the Image

```bash
# Save image
docker save nndeploy-linux -o nndeploy-linux.tar

# On another machine
docker load -i nndeploy-linux.tar
```

## Notes

* `.so` files are located at `/workspace/python/nndeploy` and are registered in `ldconfig`.
* You can modify `app.py` as needed before building.
* If you change source code, rebuild the image.

## Development Environment (Dockerfile.ort_ov_trt_mnn.develop)

The development environment uses local source code mounting for faster iteration.

### Build

```bash
docker build -f docker/Dockerfile.ort_ov_trt_mnn.develop -t nndeploy:ort_ov_trt_mnn_develop .
```

### Using the Run Script (Recommended)

```bash
# Make executable
chmod +x docker/run_develop.sh

# Start with GPU
./docker/run_develop.sh

# Start without GPU
./docker/run_develop.sh --no-gpu

# View help
./docker/run_develop.sh --help
```

### Manual Run

```bash
# With GPU
docker run -it --rm \
    --name nndeploy_dev \
    --gpus all \
    --network host \
    -v $(pwd):/workspace \
    -w /workspace \
    nndeploy:ort_ov_trt_mnn_develop \
    /bin/bash

# Without GPU
docker run -it --rm \
    --name nndeploy_dev \
    --network host \
    -v $(pwd):/workspace \
    -w /workspace \
    nndeploy:ort_ov_trt_mnn_develop \
    /bin/bash
```

### Inside the Container

After entering the container, you can:

1. **Install dependencies**:
```bash
python3 tool/script/install_opencv.py
python3 tool/script/install_onnxruntime.py
python3 tool/script/install_mnn.py
```

2. **Build the project**:
```bash
mkdir -p build && cd build
cp ../cmake/config_opencv_ort_ov_trt_mnn_tokenizer.cmake config.cmake
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ..
make -j$(nproc)
make install
```

3. **Install Python package**:
```bash
cd python && pip install -e .
echo "/workspace/python/nndeploy" >> /etc/ld.so.conf.d/nndeploy.conf
ldconfig
```

### Key Differences from Production Images

| Feature | Production Images | Development Image |
|---------|------------------|-------------------|
| Source Code | Cloned from GitHub | Mounted from host |
| Use Case | Deployment | Development/Debugging |
| File Changes | Requires rebuild | Reflects immediately |
| Persistence | Not persistent | Persistent on host |