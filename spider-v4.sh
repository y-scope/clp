set -o pipefail
export PIPX_BIN_DIR=/usr/local/bin PIPX_HOME=/opt/pipx PATH="/usr/local/bin:$PATH"
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig
pipx install --force "cmake>=4.4" >/dev/null 2>&1
hash -r
echo "##### CMake: $(cmake --version | head -1) #####"
echo "##### Building Spider deps (the chain that failed in CI) #####"
CLP_CPP_MAX_PARALLELISM_PER_BUILD_TASK=$(nproc) task deps:spider
echo "##### SPIDER DEPS EXIT=$? #####"
