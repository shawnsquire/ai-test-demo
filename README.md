# Professional Subsurface Scattering Demo

A real-time subsurface scattering demo implementing Disney/Burley BRDF and Hanrahan-Krueger subsurface scattering models on professional 3D scenes.

## Features
- ✅ Industry-standard subsurface scattering with Disney diffuse model
- ✅ Professional model loading (Sponza, Bistro, Stanford models)
- ✅ Real-time global illumination with Monte Carlo sampling
- ✅ Controllable material parameters (scattering, absorption, thickness)
- ✅ Dynamic lighting with multiple light sources
- ✅ Camera controls for scene exploration

## Arch Linux Setup

### 1. Install Required Packages
```sh
# Core development tools
sudo pacman -S base-devel cmake git

# OpenGL and windowing
sudo pacman -S mesa opengl-man-pages glfw glew

# Math and model loading
sudo pacman -S glm assimp

# Optional: Download tools
sudo pacman -S wget curl unzip
```

### 2. Clone and Build
```sh
# Clone the project
git clone <your-repo-url>
cd subsurface-scattering-demo

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)
```

### 3. Set Up Professional Models
```sh
# Create models directory from project root
mkdir -p models
cd models

# Stanford Lucy
wget https://casual-effects.com/g3d/data10/research/model/lucy/lucy.obj

# Stanford Bunny
wget https://casual-effects.com/g3d/data10/research/model/bunny/bunny.obj

# Intel Sponza
mkdir -p sponza
cd sponza
wget https://casual-effects.com/g3d/data10/research/model/sponza/sponza.zip
unzip sponza.zip
cd ..

# Lumberyard Bistro (manual download)
mkdir -p bistro
cd bistro
echo "Download Bistro manually from Amazon Lumberyard and extract here"
cd ..
cd ..
```

### 4. Alternative Model Sources
```sh
cd models

# Dragon Model
wget https://casual-effects.com/g3d/data10/research/model/dragon/dragon.obj

# GitHub Repository
git clone https://github.com/alecjacobson/common-3d-test-models.git
cp common-3d-test-models/data/*.obj ./
```

### 5. Expected Directory Structure
```
subsurface-scattering-demo/
├── main.cpp
├── CMakeLists.txt
├── README.md
├── build/
│   └── sss_demo
└── models/
    ├── lucy.obj
    ├── bunny.obj
    ├── dragon.obj
    └── sponza/
        └── sponza.obj
```

## Running the Demo
```sh
# From project root
cd build
./sss_demo
```

### Controls
- **WASD**: Move camera
- **Mouse**: Look around (if implemented)
- **ESC**: Exit

### What You'll See
- Complex geometry with realistic subsurface scattering
- Dynamic lighting
- Skin-like materials with light penetration
- Console performance metrics

## Troubleshooting

### Model Loading Issues
```sh
# Check if models exist
ls -la models/

# Check model sizes
du -h models/*.obj
```

### Graphics Driver Issues
```sh
# OpenGL check
glxinfo | grep "OpenGL version"

# Driver installation
sudo pacman -S mesa         # Intel/AMD
sudo pacman -S nvidia nvidia-utils  # NVIDIA
```

### Build Errors
```sh
cd build
make clean
cmake ..
make -j$(nproc)

ldd ./sss_demo  # Check for missing dependencies
```

### Performance Issues
```sh
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./sss_demo
./sss_demo 2>&1 | grep "Frame time"
```

## Technical Details

### Implemented Algorithms
- Disney Diffuse BRDF with subsurface term
- Hanrahan-Krueger BRDF
- Monte Carlo GI with hemisphere sampling
- Beer's Law for light absorption
- ACES Tone Mapping

### Material Parameters
- `scatteringCoeff` (RGB): Wavelength-dependent scattering
- `absorptionCoeff` (RGB): Material absorption properties
- `scatteringDistance`: Light penetration depth
- `internalColor`: Subsurface material color
- `thickness`: Material thickness for transmission
- `roughness`: Surface roughness (α in Disney model)
- `subsurfaceMix`: Blend factor (kss in Disney model)

## References
- *Real-Time Rendering*, 4th Edition (Akenine-Möller et al.)
- Disney Principled BRDF (Burley, 2012)
- Hanrahan-Krueger Subsurface BRDF (1993)

## Model Credits
- **Stanford Lucy**: Stanford Computer Graphics Laboratory
- **Stanford Bunny**: Stanford Computer Graphics Laboratory
- **Intel Sponza**: Intel Corporation (Crytek original)
- **Lumberyard Bistro**: Amazon Lumberyard

These models are used for educational and research purposes under their respective licenses.

---

**Expected Output**: Realistic subsurface scattering on complex geometry with industry-standard material parameters and professional lighting models.
