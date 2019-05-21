#include "include/jabcode.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include "generated/Chunk.pb.h"
#include <sstream>
#include <cstring>

using namespace std;

size_t getFileSizeBytes(const std::string fileName)
{
    struct stat results;
    if (stat(fileName.c_str(), &results) == 0)
        return results.st_size;
    return 0;
}

jab_data *wrapHeaderChunk(char *buffer, string fileName, unsigned int no_pieces, unsigned int size) //wrap header
{
    Header *header = new Header(); //initializes protobuf container (read the protobuf c++ docs for more info)
    header->set_file_name(fileName);
    header->set_piece_no(0);
    header->set_no_pieces(no_pieces);

    string string_buffer(buffer, size); //create a string container of size "size" and store the contents of buffer

    header->set_payload(string_buffer);
    header->set_payload_size(size);

    ostringstream stream; // creating a string stream to allow us convert from Header to a string container
    header->SerializeToOstream(&stream);

    string temp_string = stream.str();

    jab_data *data = 0;                                                                    //jab_data is used by the jabcode library to store data
    data = (jab_data *)malloc(sizeof(jab_data) + temp_string.length() * sizeof(jab_char)); // allocating memory to store temp_string as a jab_data
    data->length = temp_string.length();

    memcpy(data->data, temp_string.c_str(), data->length); //read on memcpy

    return data;
}

jab_data *wrapDataChunk(char *buffer, unsigned int piece_no, unsigned int size) //add header information to the payload
{
    Data *data = new Data();
    data->set_piece_no(piece_no);

    string string_buffer(buffer, size);
    data->set_payload(string_buffer);

    data->set_payload_size(size);

    ostringstream stream;
    data->SerializeToOstream(&stream);
    string temp_string = stream.str();

    jab_data *jdata = 0;
    jdata = (jab_data *)malloc(sizeof(jab_data) + temp_string.length() * sizeof(jab_char));
    jdata->length = temp_string.length();

    memcpy(jdata->data, temp_string.c_str(), jdata->length);

    return jdata;
}

int chunkFile(char *fullFilePath, string chunkName, unsigned long chunkSize)
{
    ifstream fileStream;
    fileStream.open(fullFilePath, ios::in | ios::binary);

    // File open a success
    if (!fileStream.is_open()) //check if input file does not exist
    {
        std::cout << "Error opening input file!" << endl;
        return 1;
    }

    std::cout << "opened: " << fullFilePath << endl;

    int counter = 0;

    // Create a buffer to hold each chunk
    char *buffer = new char[chunkSize];

    // Keep reading until end of file
    while (!fileStream.eof())
    {

        string fullChunkName = chunkName + to_string(counter) + ".png";

        fileStream.read(buffer, chunkSize);

        const jab_int32 symbol_number = 145; //read the jabcode specification, table 1 for more information
        const jab_int32 color_number = 256;

        jab_encode *enc = createEncode(symbol_number, color_number);
        jab_data *data = 0;

        if (counter == 0) //the header has a piece no of 0
        {
            uint32_t no_pieces = 1 + ((getFileSizeBytes(fullFilePath) - 1) / chunkSize); //  number of pieces needed to be encoded including the header
            cout << "Encoding: " << no_pieces << " Pieces" << endl;

            data = wrapHeaderChunk(buffer, fullFilePath, no_pieces, fileStream.gcount());
        }
        else
        {
            data = wrapDataChunk(buffer, counter, fileStream.gcount());
        }

        std::cout << "encoding chunk: " << counter << endl;

        if (generateJABCode(enc, data) != 0)
        {
            std::cout << "jabcode not generated" << endl;
            return 1;
        }

        //converting from a string (fullChunkName) to char* (file_name)
        jab_char *file_name = new jab_char[fullChunkName.length() + 1];
        strcpy(file_name, fullChunkName.c_str());

        saveImage(enc->bitmap, file_name);

        //deallocate memory
        destroyEncode(enc);
        delete (file_name);

        counter++;
    }

    // deallocate buffer
    delete (buffer);

    // Close input file stream.
    fileStream.close();

    std::cout << "Chunking complete! " << counter << " barcodes generated." << endl;
}

int joinFile(string chunkName)
{

    string headerChunkName = chunkName + to_string(0) + ".png";
    jab_bitmap *bitmap;
    bitmap = readImage((char *)headerChunkName.c_str()); //read  header jabcode
    jab_int32 status;
    jab_data *decoded_data = decodeJABCode(bitmap, NORMAL_DECODE, &status);

    if (status != 3)
    {
        std::cout << "Error: Unable to decode header file." << endl;
        return 1;
    }

    std::cout << "Decoding Header Chunk" << endl;

    string stringBuffer(decoded_data->data, decoded_data->length);

    Header *header = new Header();
    header->ParseFromString(stringBuffer); //converting jabcode to protobuf container

    char *inputHeaderBuffer = new char[header->payload_size()]; //allocating space for the payload
    memcpy(inputHeaderBuffer, header->payload().c_str(), header->payload_size());
    string fileOutputName = "output_" + header->file_name();
    cout << "saving to " << fileOutputName << endl;
    ofstream outputfile;
    outputfile.open(fileOutputName.c_str(), ios::out | ios::binary);

    if (!outputfile.is_open())
    {
        std::cout << "Error: Unable to open file for output." << endl;
        return 1;
    }

    outputfile.write(inputHeaderBuffer, header->payload_size());
    delete[] inputHeaderBuffer;

    cout << "No Of Pieces: " << header->no_pieces() << endl;

    for (int counter = 1; counter < header->no_pieces(); counter++)
    {
        string fullChunkName = chunkName + to_string(counter) + ".png";

        jab_bitmap *bitmap;
        bitmap = readImage((char *)fullChunkName.c_str());
        jab_int32 status;
        jab_data *decoded_data = decodeJABCode(bitmap, NORMAL_DECODE, &status);
        if (status != 3)
        {
            std::cout << "Unable to decode chunk: " << counter << endl;
            return 1;
        }

        std::cout << "Decoding chunk: " << counter << endl;

        string stringBuffer(decoded_data->data, decoded_data->length);

        Data *data = new Data();             //create Protobuf container
        data->ParseFromString(stringBuffer); //initialize probuf container with decoded data

        char *inputBuffer = new char[data->payload_size()]; //create a container for storing payload
        memcpy(inputBuffer, data->payload().c_str(), data->payload_size());

        outputfile.write(inputBuffer, data->payload_size());
        delete[] inputBuffer;
    }

    // Close output file.
    outputfile.close();

    std::cout << "File assembly complete!" << endl;
}

int main()
{
    chunkFile("picture.jpg", "./barcodes/chunk", 3000); //takes the file to be encoded, and directory to store the barcodes (it should be created manually)
    joinFile("./barcodes/chunk");                       //takes the barcode directory
    return 0;
}