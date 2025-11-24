$CXX = "C:\msys64\ucrt64\bin\g++.exe"
$CXXFLAGS = @("-std=c++17", "-O3", "-march=native", "-Wall", "-Wextra", "-I", "include")
$SRC_DIR = "src"
$BUILD_DIR = "build"

Write-Host "Compiling source files..." -ForegroundColor Cyan

$sources = @(
    "$SRC_DIR/iex_parser.cpp",
    "$SRC_DIR/itch_parser.cpp",
    "$SRC_DIR/order_book.cpp",
    "$SRC_DIR/enhanced_order_book.cpp",
    "$SRC_DIR/websocket_server.cpp",
    "$SRC_DIR/tick_recorder.cpp",
    "$SRC_DIR/replay_engine.cpp",
    "$SRC_DIR/strategy.cpp",
    "$SRC_DIR/pcap_reader.cpp"
)

$objects = @()

foreach ($src in $sources) {
    $obj = $src -replace ".cpp$", ".o" -replace "src/", "$BUILD_DIR/"
    Write-Host "Compiling $src -> $obj"
    & $CXX @CXXFLAGS -c $src -o $obj
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Compilation failed for $src" -ForegroundColor Red
        exit 1
    }
    $objects += $obj
}

Write-Host "`nLinking main executable..." -ForegroundColor Cyan
& $CXX @CXXFLAGS @objects "$SRC_DIR/main.cpp" -o "$BUILD_DIR/feedhandler.exe" -lws2_32

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nBuild successful!" -ForegroundColor Green
    Write-Host "Executable: $BUILD_DIR/feedhandler.exe"
} else {
    Write-Host "`nBuild failed!" -ForegroundColor Red
    exit 1
}
