#include "include/jabcode.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include "generated/Chunk.pb.h"
#include <sstream>
#include <cstring>

using namespace std;

jab_data *wrapHeaderChunk(char *buffer, string fileName, unsigned int no_pieces, unsigned int size)
{
    Header *header = new Header();
    header->set_file_name(fileName);
    header->set_piece_no(0);
    header->set_no_pieces(no_pieces);

    string string_buffer(buffer, size);
    cout << "Input size: " << string_buffer.size() << endl;
    cout << "passed size: " << size << endl;
    header->set_payload(string_buffer);

    header->set_payload_size(size);
    ostringstream stream;
    header->SerializeToOstream(&stream);
    string temp_string = stream.str();

    jab_data *data = 0;
    data = (jab_data *)malloc(sizeof(jab_data) + temp_string.length() * sizeof(jab_char));
    data->length = temp_string.length();

    memcpy(data->data, temp_string.c_str(), data->length);
    // delete (&string_buffer);
    return data;
}

jab_data *wrapDataChunk(char *buffer, unsigned int piece_no, unsigned int size)
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
    // delete (&string_buffer);
    return jdata;
}

void chunkFile(char *fullFilePath, string chunkName, unsigned long chunkSize)
{
    ifstream fileStream;
    fileStream.open(fullFilePath, ios::in | ios::binary);

    // File open a success
    if (fileStream.is_open())
    {
        cout << "opened: " << fullFilePath << endl;
        ofstream output;
        int counter = 0;

        // Create a buffer to hold each chunk
        char *buffer = new char[chunkSize];

        // Keep reading until end of file
        while (!fileStream.eof())
        {

            string fullChunkName = chunkName + to_string(counter) + ".png";

            fileStream.read(buffer, chunkSize);

            const jab_int32 symbol_number = 145;
            const jab_int32 color_number = 256;

            jab_encode *enc = createEncode(symbol_number, color_number);
            jab_data *data = 0;

            if (counter == 0)
            {
                /* code */
                data = wrapHeaderChunk(buffer, fullFilePath, 10, fileStream.gcount());
            }
            else
            {
                // cout << fileStream.gcount() << endl;
                data = wrapDataChunk(buffer, counter, fileStream.gcount());
            }

            cout << "encoding chunk: " << counter << endl;
            if (generateJABCode(enc, data) != 0)
            {
                cout << "jabcode not generated" << endl;
            }

            jab_char *file_name = new jab_char[fullChunkName.length() + 1];
            strcpy(file_name, fullChunkName.c_str());
            saveImage(enc->bitmap, file_name);
            destroyEncode(enc);
            delete (file_name);
            counter++;
        }

        // Cleanup buffer
        delete (buffer);

        // Close input file stream.
        fileStream.close();
        cout << "Chunking complete! " << counter << " files created." << endl;
    }
    else
    {
        cout << "Error opening file!" << endl;
    }
}

// Finds chunks by "chunkName" and creates file specified in fileOutput
void joinFile(string chunkName, string fileOutput)
{

    // Create our output file
    ofstream outputfile;
    outputfile.open(fileOutput, ios::out | ios::binary);

    // If successful, loop through chunks matching chunkName
    if (outputfile.is_open())
    {
        bool filefound = true;
        int counter = 0;

        while (filefound)
        {
            filefound = false;

            // Open chunk to read
            string fullChunkName = chunkName + to_string(counter) + ".png";
            ifstream fileInput;
            fileInput.open(fullChunkName, ios::in | ios::binary);
            // If chunk opened successfully, read it and write it to
            // output file.
            if (fileInput.is_open())
            {
                filefound = true;
                jab_bitmap *bitmap;
                bitmap = readImage((char *)fullChunkName.c_str());
                jab_int32 status;
                jab_data *decoded_data = decodeJABCode(bitmap, NORMAL_DECODE, &status);
                if (status == 3) //successfully decoded
                {
                    // char *inputBuffer = new char[decoded_data->length];
                    if (counter == 0)
                    {

                        cout << "Decoding Header Chunk" << endl;

                        string stringBuffer(decoded_data->data, decoded_data->length);

                        Header *header = new Header();
                        header->ParseFromString(stringBuffer);

                        char *inputBuffer = new char[header->payload_size()];
                        memcpy(inputBuffer, header->payload().c_str(), header->payload_size());

                        outputfile.write(inputBuffer, header->payload_size());
                    }
                    else
                    {
                        cout << "Decoding chunk: " << counter << endl;

                        string stringBuffer(decoded_data->data, decoded_data->length);

                        Data *data = new Data();             //create Protobuf container
                        data->ParseFromString(stringBuffer); //initialize probuf container with decoded data

                        char *inputBuffer = new char[data->payload_size()]; //create a container for storing payload
                        memcpy(inputBuffer, data->payload().c_str(), data->payload_size());

                        outputfile.write(inputBuffer, data->payload_size());
                    }

                    fileInput.close();
                }
                else
                {
                    cout << "Unable to decode" << endl;
                    return;
                }
            }
            counter++;
        }

        // Close output file.
        outputfile.close();

        cout << "File assembly complete!" << endl;
    }
    else
    {
        cout << "Error: Unable to open file for output." << endl;
    }
}

int main()
{
    chunkFile("./1.png", "./barcodes/chunk", 3000);
    joinFile("./barcodes/chunk", "./joined.png");
    return 0;
}