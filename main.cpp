#include "include/jabcode.h"
#include "include/xxhash.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include "generated/Chunk.pb.h"
#include <sstream>

using namespace std;

int encode(jab_char *, string);
int decode(string);

xxh::hash32_t generateHash(char *data_string)
{
    xxh::hash_t<32> hash = xxh::xxhash<32, char>(data_string);

    return hash;
}

string wrapHeaderChunk(char *buffer, string fileName, unsigned int no_pieces)
{
    Header *header = new Header();
    header->set_file_name(fileName);
    header->set_piece_no(0);
    header->set_no_pieces(no_pieces);
    header->set_payload(buffer);
    ostringstream stream;
    header->SerializeToOstream(&stream);
    string temp_string = stream.str();
    return temp_string;
}

string wrapDataChunk(char *buffer, unsigned int piece_no)
{
    Data *data = new Data();
    data->set_piece_no(piece_no);
    data->set_payload(buffer);
    ostringstream stream;
    data->SerializeToOstream(&stream);
    string temp_string = stream.str();
    return temp_string;
}

size_t getFileSizeBytes(const std::string fileName)
{
    struct stat results;
    if (stat(fileName.c_str(), &results) == 0)
        return results.st_size;
    return 0;
}

bool encodeFile(const string fileName, unsigned int buffer_size)
{
    unsigned int chunkCount = 0;

    char *buffer = new char[buffer_size];

    ifstream fileStream;
    fileStream.open(fileName, ios::in | ios::binary);

    if (!fileStream.is_open())
    {
        cout << "unable to open " << fileName << endl;
        return 1;
    }

    while (!fileStream.eof())
    {

        fileStream.read(buffer, buffer_size);
        string chunk_name = to_string(chunkCount);

        if (chunkCount == 0)
        { //header

            cout << "size of file (kb): " << (float)getFileSizeBytes(fileName) / 1000 << endl;
            uint32_t no_pieces = 1 + ((getFileSizeBytes(fileName) - 1) / buffer_size);
            cout << "No of pieces to encode: " << no_pieces << endl;

            string wrappedChunk = wrapHeaderChunk(buffer, fileName, no_pieces);
            const char *c = wrappedChunk.c_str();

            if (encode((char *)c, chunk_name) != 0)
            {
                cout << "Unable to encode chunk: " << chunkCount << endl;
                break;
            }
        }
        else
        {

            string wrappedChunk = wrapDataChunk(buffer, chunkCount);
            const char *c = wrappedChunk.c_str();
            if (encode((char *)c, chunk_name) != 0)
            {
                cout << "Unable to encode chunk: " << chunkCount << endl;
                break;
            }
        }

        cout << "Encoded Chunk: " << chunk_name << endl;
        chunkCount++;
    }
    cout << "Created: " << chunkCount << " Chunks." << endl;
    delete (buffer);
}

int encode(jab_char *chunk_data, string chunk_name)
{
    const jab_int32 symbol_number = 145;
    const jab_int32 color_number = 256;

    jab_encode *enc = createEncode(symbol_number, color_number);

    jab_data *data = 0;
    data = (jab_data *)malloc(sizeof(jab_data) + strlen(chunk_data) * sizeof(jab_char));
    data->length = strlen(chunk_data);
    memcpy(data->data, chunk_data, data->length);

    if (generateJABCode(enc, data) != 0)
    {

        std::cout << "jabcode not generated" << std::endl;
        return 1;
    };

    chunk_name = "./barcodes/" + chunk_name + ".png";
    jab_char *file_name = new jab_char[chunk_name.length() + 1]; //allocate char* space for string
    strcpy(file_name, chunk_name.c_str());                       //copy string to char*

    saveImage(enc->bitmap, file_name);
    destroyEncode(enc);

    return 0;
}

char *decodeImage(string imageName)
{
    jab_bitmap *bitmap;
    bitmap = readImage((char *)imageName.c_str());
    jab_int32 status;
    jab_data *decoded_data = decodeJABCode(bitmap, NORMAL_DECODE, &status);
    if (status == 3)
    {
        return decoded_data->data;
    }
    else
    {
        cout << "unable to decode" << endl;
    }
    delete (bitmap);
}

int main()
{

    // Header *header = new Header();
    string data("hel\0lo", 6);

    char to[1000];
    copy(data.begin(), data.end(), begin(to));
    // header->set_payload(data);
    ofstream fileOutput;

    fileOutput.open("out.bin", ios::out | ios::binary);
    fileOutput << to;
}

// g++ main.cpp ./build/libjabcode.a -lpng -lprotobuf