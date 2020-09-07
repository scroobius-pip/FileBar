Filebar allows files of any type to be printed on paper. It's an alternate means of storing data, you could store a music file by running it through the program. The program generates high capacity bar codes that could  be printed on paper. 




Compilation Steps 

1. Compile Jabcode Library
2. Compile Protobuf Library
3. Install lpng library
4. Build Protobuf header definitions
5. Build Code
6. Run Code

1) Compile Jabcode Library
   a) Head over to https://github.com/jabcode/jabcode and follow the instructions to build the core library.
   b) Copy libjabcode.a into /build (replace current one if already exists)
   c) Copy jabcode.h in jabcode/include into /include

2) Install Protobuf Library
   a) http://google.github.io/proto-lens/installing-protoc.html
   b) https://github.com/protocolbuffers/protobuf/blob/master/src/README.md

3) Install lpng library
   a) run "brew install libpng" with terminal

4) Build Protobuf header definitions
   a) open the terminal in this folder run the following in the terminal: "protoc -I ./ --cpp_out ./generated ./Chunk.proto"

5) Build Code
   a)Run the following in terminal "// g++ -std=c++11 main.cpp ./generated/Chunk.pb.cc ./build/libjabcode.a -lpng  -lprotobuf -ltiff -lz -ljpeg -lpthread"

6) Run Code
   a) Run "./a.out"
