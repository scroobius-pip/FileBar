#include "include/jabcode.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sys/stat.h>
#include "generated/Chunk.pb.h"
#include <sstream>
#include <cstring>

using namespace std;

string wrapHeaderChunk(char *buffer, string fileName, unsigned int no_pieces, unsigned int size)
{
    Header *header = new Header();
    header->set_file_name(fileName);
    header->set_piece_no(0);
    header->set_no_pieces(no_pieces);
    header->set_payload(buffer);
    header->set_size(size);
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
            data = (jab_data *)malloc(sizeof(jab_data) + fileStream.gcount() * sizeof(jab_char));
            data->length = fileStream.gcount();
            memcpy(data->data, buffer, data->length);
            cout << "encoding chunk: " << counter << endl;
            if (generateJABCode(enc, data) != 0)
            {
                cout << "jabcode not generated" << endl;
            }

            // chunkName =
            jab_char *file_name = new jab_char[fullChunkName.length() + 1];
            strcpy(file_name, fullChunkName.c_str());
            saveImage(enc->bitmap, file_name);
            destroyEncode(enc);
            delete (file_name);
            counter++;
            // }
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
                    char *inputBuffer = new char[decoded_data->length];
                    outputfile.write(decoded_data->data, decoded_data->length);
                    delete (inputBuffer);
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
    chunkFile("./1.png", "./barcodes/chunk", 2000);
    joinFile("./barcodes/chunk", "./joined.png");
    return 0;
}