# Central Controller build dependencies for macOS development.
#
#   brew bundle            # install the deps below
#   ZT_CONTROLLER_DEPS=1 scripts/bootstrap-deps.sh   # build the two pinned controller deps (full OTel + google-cloud-cpp)
#
# The two version-sensitive C++ libraries (opentelemetry-cpp, google-cloud-cpp)
# are NOT taken from Homebrew -- they are pinned and built from source by
# scripts/bootstrap-deps.sh so every dev/CI machine uses the same versions
# (the role conda previously filled). Everything below is stable enough to take
# from Homebrew.

brew "cmake"
brew "ninja"
brew "pkg-config"
brew "rustup-init"      # then: rustup-init -y && rustup default stable

brew "libpqxx"
brew "libpq"           # keg-only; add $(brew --prefix libpq) to CMAKE_PREFIX_PATH so find_package(PostgreSQL) resolves
brew "hiredis"
brew "jemalloc"
brew "nlohmann-json"
brew "openssl@3"
brew "abseil"
brew "protobuf"
brew "grpc"
brew "googletest"      # google-cloud-cpp requires GTest/GMock at configure (builds its *_mocks libs)
brew "curl"
brew "inja"
