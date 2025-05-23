Professional Subsurface Scattering Demo
A real-time subsurface scattering demo implementing Disney/Burley BRDF and Hanrahan-Krueger subsurface scattering models on professional 3D scenes.

Features
✅ Industry-standard subsurface scattering with Disney diffuse model
✅ Professional model loading (Sponza, Bistro, Stanford models)
✅ Real-time global illumination with Monte Carlo sampling
✅ Controllable material parameters (scattering, absorption, thickness)
✅ Dynamic lighting with multiple light sources
✅ Camera controls for scene exploration

Arch Linux Setup
1. Install Required Packages
Text Only
# Core development tools
sudo pacman -S base-devel cmake git

# OpenGL and windowing
sudo pacman -S mesa opengl-man-pages glfw glew

# Math and model loading
sudo pacman -S glm assimp

# Optional: Download tools
sudo pacman -S wget curl unzip
2. Clone and Build
Text Only
# Clone the project
git clone &lt;your-repo-url&gt;
cd subsurface-scattering-demo

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j$(nproc)
3. Set Up Professional Models
Create the models directory and download industry-standard test models:

Text Only
# Create models directory from project root
mkdir -p models

# Stanford Lucy (high-detail scan)
cd models
wget https://casual-effects.com/g3d/data10/research/model/lucy/lucy.obj
# Alternative: wget http://graphics.stanford.edu/pub/3Dscanrep/lucy/lucy.ply

# Stanford Bunny
wget https://casual-effects.com/g3d/data10/research/model/bunny/bunny.obj
# Alternative: curl -O https://github.com/alecjacobson/common-3d-test-models/raw/master/data/bunny.obj

# Intel Sponza (updated Crytek Sponza)
mkdir -p sponza
cd sponza
wget https://casual-effects.com/g3d/data10/research/model/sponza/sponza.zip
unzip sponza.zip
cd ..

# Lumberyard Bistro (if available)
mkdir -p bistro
cd bistro
# Note: Bistro requires manual download from Amazon Lumberyard
# Visit: https://docs.aws.amazon.com/lumberyard/latest/userguide/sample-level-bistro.html
echo &quot;Download Bistro manually from Amazon Lumberyard and extract here&quot;
cd ..

cd .. # Back to project root
4. Alternative Model Sources
If the above links don't work, try these alternatives:

Text Only
cd models

# McGuire Computer Graphics Archive (excellent source)
# Visit: https://casual-effects.com/data/
wget https://casual-effects.com/g3d/data10/research/model/dragon/dragon.obj

# Sketchfab (requires account but has many models)
# Visit: https://sketchfab.com/3d-models/stanford-bunny-

# GitHub repositories with test models
git clone https://github.com/alecjacobson/common-3d-test-models.git
cp common-3d-test-models/data/*.obj ./
5. Expected Directory Structure
Your project should look like this:

Text Only
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
Running the Demo
Basic Execution
Text Only
# From project root
cd build
./sss_demo
Controls
WASD: Move camera around the scene
Mouse: Look around (if mouse capture is implemented)
ESC: Exit the demo
What You'll See
Complex geometry with realistic subsurface scattering
Dynamic lighting that moves around the scene
Skin-like materials with proper light penetration
Performance metrics in console output
Troubleshooting
Model Loading Issues
Text Only
# Check if models exist
ls -la models/

# Check model file sizes (should not be 0 bytes)
du -h models/*.obj

# Test with procedural geometry only
# The demo will auto-generate spheres if no models are found
Graphics Driver Issues
Text Only
# Check OpenGL support
glxinfo | grep &quot;OpenGL version&quot;

# Should show OpenGL 3.3 or higher
# If not, install proper graphics drivers:

# For Intel graphics
sudo pacman -S mesa

# For NVIDIA
sudo pacman -S nvidia nvidia-utils

# For AMD
sudo pacman -S mesa xf86-video-amdgpu
Build Errors
Text Only
# Clean and rebuild
cd build
make clean
cmake ..
make -j$(nproc)

# Check for missing dependencies
ldd ./sss_demo
Performance Issues
Text Only
# Check if running on dedicated GPU (for laptops)
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./sss_demo

# Monitor performance
./sss_demo 2&gt;&amp;1 | grep &quot;Frame time&quot;
Technical Details
Implemented Algorithms
Disney Diffuse BRDF with subsurface scattering term
Hanrahan-Krueger BRDF for translucent materials
Monte Carlo Global Illumination with hemisphere sampling
Beer's Law for realistic light absorption
ACES Tone Mapping for professional color grading
Material Parameters
The demo exposes these physically-based parameters:

scatteringCoeff: Wavelength-dependent scattering (RGB)
absorptionCoeff: Material absorption properties (RGB)
scatteringDistance: Light penetration depth
internalColor: Subsurface material color
thickness: Material thickness for transmission
roughness: Surface roughness (α in Disney model)
subsurfaceMix: Blend factor (kss in Disney model)
References
Real-Time Rendering, 4th Edition (Akenine-Möller et al.)
Disney Principled BRDF (Burley, 2012)
Hanrahan-Krueger Subsurface BRDF (1993)
Model Credits
Stanford Lucy: Stanford Computer Graphics Laboratory
Stanford Bunny: Stanford Computer Graphics Laboratory
Intel Sponza: Intel Corporation (Crytek original)
Lumberyard Bistro: Amazon Lumberyard
These models are used for educational and research purposes under their respective licenses.

Expected Output: Realistic subsurface scattering on complex geometry with industry-standard material parameters and professional lighting models.
