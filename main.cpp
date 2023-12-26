#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <string>
#include <cctype>
#include <algorithm>
#include <limits>
#include <sstream>
using namespace std;

// Pixel structure
struct Pixel
{
    // Red, green, blue color values
    int red;
    int green;
    int blue;
};

/**
 * Gets an integer from a binary stream.
 * Helper function for read_image()
 * @param stream the stream
 * @param offset the offset at which to read the integer
 * @param bytes  the number of bytes to read
 * @return the integer starting at the given offset
 */
int get_int(fstream &stream, int offset, int bytes)
{
    stream.seekg(offset);
    int result = 0;
    int base = 1;
    for (int i = 0; i < bytes; i++)
    {
        result = result + stream.get() * base;
        base = base * 256;
    }
    return result;
}

/**
 * Reads the BMP image specified and returns the resulting image as a vector
 * @param filename BMP image filename
 * @return the image as a vector of vector of Pixels
 */
vector<vector<Pixel>> read_image(string filename)
{
    // Open the binary file
    fstream stream;
    stream.open(filename, ios::in | ios::binary);

    // Get the image properties
    int file_size = get_int(stream, 2, 4);
    int start = get_int(stream, 10, 4);
    int width = get_int(stream, 18, 4);
    int height = get_int(stream, 22, 4);
    int bits_per_pixel = get_int(stream, 28, 2);

    // Scan lines must occupy multiples of four bytes
    int scanline_size = width * (bits_per_pixel / 8);
    int padding = 0;
    if (scanline_size % 4 != 0)
    {
        padding = 4 - scanline_size % 4;
    }

    // Return empty vector if this is not a valid image
    if (file_size != start + (scanline_size + padding) * height)
    {
        return {};
    }

    // Create a vector the size of the input image
    vector<vector<Pixel>> image(height, vector<Pixel>(width));

    int pos = start;
    // For each row, starting from the last row to the first
    // Note: BMP files store pixels from bottom to top
    for (int i = height - 1; i >= 0; i--)
    {
        // For each column
        for (int j = 0; j < width; j++)
        {
            // Go to the pixel position
            stream.seekg(pos);

            // Save the pixel values to the image vector
            // Note: BMP files store pixels in blue, green, red order
            image[i][j].blue = stream.get();
            image[i][j].green = stream.get();
            image[i][j].red = stream.get();

            // We are ignoring the alpha channel if there is one

            // Advance the position to the next pixel
            pos = pos + (bits_per_pixel / 8);
        }

        // Skip the padding at the end of each row
        stream.seekg(padding, ios::cur);
        pos = pos + padding;
    }

    // Close the stream and return the image vector
    stream.close();
    return image;
}

/**
 * Sets a value to the char array starting at the offset using the size
 * specified by the bytes.
 * This is a helper function for write_image()
 * @param arr    Array to set values for
 * @param offset Starting index offset
 * @param bytes  Number of bytes to set
 * @param value  Value to set
 * @return nothing
 */
void set_bytes(unsigned char arr[], int offset, int bytes, int value)
{
    for (int i = 0; i < bytes; i++)
    {
        arr[offset + i] = (unsigned char)(value >> (i * 8));
    }
}

/**
 * Write the input image to a BMP file name specified
 * @param filename The BMP file name to save the image to
 * @param image    The input image to save
 * @return True if successful and false otherwise
 */
bool write_image(string filename, const vector<vector<Pixel>> &image)
{
    // Get the image width and height in pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Calculate the width in bytes incorporating padding (4 byte alignment)
    int width_bytes = width_pixels * 3;
    int padding_bytes = 0;
    padding_bytes = (4 - width_bytes % 4) % 4;
    width_bytes = width_bytes + padding_bytes;

    // Pixel array size in bytes, including padding
    int array_bytes = width_bytes * height_pixels;

    // Open a file stream for writing to a binary file
    fstream stream;
    stream.open(filename, ios::out | ios::binary);

    // If there was a problem opening the file, return false
    if (!stream.is_open())
    {
        return false;
    }

    // Create the BMP and DIB Headers
    const int BMP_HEADER_SIZE = 14;
    const int DIB_HEADER_SIZE = 40;
    unsigned char bmp_header[BMP_HEADER_SIZE] = {0};
    unsigned char dib_header[DIB_HEADER_SIZE] = {0};

    // BMP Header
    set_bytes(bmp_header, 0, 1, 'B');                                             // ID field
    set_bytes(bmp_header, 1, 1, 'M');                                             // ID field
    set_bytes(bmp_header, 2, 4, BMP_HEADER_SIZE + DIB_HEADER_SIZE + array_bytes); // Size of BMP file
    set_bytes(bmp_header, 6, 2, 0);                                               // Reserved
    set_bytes(bmp_header, 8, 2, 0);                                               // Reserved
    set_bytes(bmp_header, 10, 4, BMP_HEADER_SIZE + DIB_HEADER_SIZE);              // Pixel array offset

    // DIB Header
    set_bytes(dib_header, 0, 4, DIB_HEADER_SIZE); // DIB header size
    set_bytes(dib_header, 4, 4, width_pixels);    // Width of bitmap in pixels
    set_bytes(dib_header, 8, 4, height_pixels);   // Height of bitmap in pixels
    set_bytes(dib_header, 12, 2, 1);              // Number of color planes
    set_bytes(dib_header, 14, 2, 24);             // Number of bits per pixel
    set_bytes(dib_header, 16, 4, 0);              // Compression method (0=BI_RGB)
    set_bytes(dib_header, 20, 4, array_bytes);    // Size of raw bitmap data (including padding)
    set_bytes(dib_header, 24, 4, 2835);           // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 28, 4, 2835);           // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 32, 4, 0);              // Number of colors in palette
    set_bytes(dib_header, 36, 4, 0);              // Number of important colors

    // Write the BMP and DIB Headers to the file
    stream.write((char *)bmp_header, sizeof(bmp_header));
    stream.write((char *)dib_header, sizeof(dib_header));

    // Initialize pixel and padding
    unsigned char pixel[3] = {0};
    unsigned char padding[3] = {0};

    // Pixel Array (Left to right, bottom to top, with padding)
    for (int h = height_pixels - 1; h >= 0; h--)
    {
        for (int w = 0; w < width_pixels; w++)
        {
            // Write the pixel (Blue, Green, Red)
            pixel[0] = image[h][w].blue;
            pixel[1] = image[h][w].green;
            pixel[2] = image[h][w].red;
            stream.write((char *)pixel, 3);
        }
        // Write the padding bytes
        stream.write((char *)padding, padding_bytes);
    }

    // Close the stream and return true
    stream.close();
    return true;
}

// Input filename check
// bool set to false - when false user has the option to quit.
// when bool == true, the user has the option to cancel (used in case 0)
string getValidBMPFilename(bool allowCancel = false)
{
    string filename;
    char choice;

    while (true)
    {
        // cancel or quit?
        if (allowCancel)
        {
            cout << "Enter new BMP filename (or type 'C' to cancel): ";
        }
        else
        {
            cout << "Enter input BMP filename (or type 'Q' to quit): ";
        }

        cin >> filename;

        // Cancel option as used in case 0
        if (allowCancel && (filename == "C" || filename == "c"))
        {
            cout << "Action cancelled. \n"
                 << endl;
            return "";
            // Quit option as used in initial file upload
        }
        else if (!allowCancel && (filename == "Q" || filename == "q"))
        {
            cout << "The application will now terminate. Goodbye! \n"
                 << endl;
            return "";
        }
        else if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".bmp")
        {
            cout << "Filename entered successfully \n"
                 << endl;
            return filename;
        }
        else
        {
            cerr << "Error: filename must end in .bmp" << endl;
            cout << "Would you like to try again (Y / N): ";
            cin >> choice;

            if (tolower(choice) == 'n')
            {
                cout << "The application will now terminate. Goodbye! \n"
                     << endl;
                return "";
            }
        }
    }
}

// Output filename check
string getValidBMPFilenameOutput()
{
    string filename;
    char choice;

    while (true)
    {
        cout << "Enter output BMP filename (or type 'C' to cancel): ";
        cin >> filename;

        if (filename == "C" || filename == "c")
        {
            cout << "Action cancelled. \n"
                 << endl;
            return "";
        }

        if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".bmp")
        {
            return filename;
        }
        else
        {
            cerr << "Error: filename must end in .bmp" << endl;
            cout << "Would you like to try again (Y / N): ";
            cin >> choice;

            if (tolower(choice) == 'n')
            {
                cout << "Action cancelled." << endl;
                return "";
            }
        }
    }
}

// check for cancellation
bool cancellationCheck(const string &filename)
{
    if (filename.empty())
    {
        return true;
    }
    return false; // cancellation has not been selected by the user
}

// Process 1 - vignette effect
vector<vector<Pixel>> process_1(const vector<vector<Pixel>> &image)
{
    // Get the number of rows/columns from the input 2D vector (remember: num_rows is height, num_columns is width)
    int num_rows = image.size();
    int num_columns = image[0].size();

    // Calculating the center of the image
    double center_row = num_rows / 2;
    double center_col = num_columns / 2;

    // Define a new 2D vector the same size as the input 2D vector
    vector<vector<Pixel>> new_image(num_rows, vector<Pixel>(num_columns));
    // For each of the rows in the input 2D vector
    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            // Distance to the center
            double distance = sqrt(pow(col - center_col, 2) + pow(row - center_row, 2));
            double scaling_factor = (num_rows - distance) / num_rows;

            // Scale new color values
            int new_red = static_cast<int>(image[row][col].red * scaling_factor);
            int new_green = static_cast<int>(image[row][col].green * scaling_factor);
            int new_blue = static_cast<int>(image[row][col].blue * scaling_factor);

            // Set new pixel values
            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }

    return new_image;
}

// Process 2 - clarendon effect
vector<vector<Pixel>> process_2(const vector<vector<Pixel>> &image, double scaling_factor)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel>(num_columns));

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            const Pixel &p = image[row][col];
            int red_value = p.red;
            int green_value = p.green;
            int blue_value = p.blue;

            // Average the values
            int average_value = (red_value + green_value + blue_value) / 3;

            int new_red, new_green, new_blue;

            // Adjusting pixel brightness according to average_value
            if (average_value >= 170)
            {
                new_red = static_cast<int>(255 - (255 - red_value) * scaling_factor);
                new_green = static_cast<int>(255 - (255 - green_value) * scaling_factor);
                new_blue = static_cast<int>(255 - (255 - blue_value) * scaling_factor);
            }
            else if (average_value < 90)
            {
                new_red = static_cast<int>(red_value * scaling_factor);
                new_green = static_cast<int>(green_value * scaling_factor);
                new_blue = static_cast<int>(blue_value * scaling_factor);
            }
            else
            {
                new_red = red_value;
                new_green = green_value;
                new_blue = blue_value;
            }

            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
    return new_image;
}

// Process 3 - grayscale image
vector<vector<Pixel>> process_3(const vector<vector<Pixel>> &image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel>(num_columns));

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            const Pixel &p = image[row][col];
            int red_value = p.red;
            int green_value = p.green;
            int blue_value = p.blue;

            // Calculate Gray Value
            int gray_value = (red_value + green_value + blue_value) / 3;

            // Set pixels to gray value

            int new_red = gray_value;
            int new_green = gray_value;
            int new_blue = gray_value;

            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }

    return new_image;
}

// Process 4 - rotates image by 90 degrees clockwise (not counter-clockwise)
vector<vector<Pixel>> process_4(const vector<vector<Pixel>> &image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    vector<vector<Pixel>> new_image(num_columns, vector<Pixel>(num_rows));

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            const Pixel &p = image[row][col];

            // 90 degree rotation logic
            new_image[col][num_rows - 1 - row] = p;
        }
    }

    return new_image;
}

// Process 5 - rotates image clockwise by a specified number of multiples of 90 degrees
vector<vector<Pixel>> process_5(const vector<vector<Pixel>> &image, int number)
{
    vector<vector<Pixel>> new_image = image;

    // Normalize the number of rotations for optimization
    number = number % 4;

    for (int i = 0; i < number; ++i)
    {
        new_image = process_4(new_image);
    }

    return new_image;
}

// Process 6 - enlarges the image in the x and y direction
vector<vector<Pixel>> process_6(const vector<vector<Pixel>> &image, int x_scale, int y_scale)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    int new_height = num_rows * y_scale;
    int new_width = num_columns * x_scale;

    vector<vector<Pixel>> new_image(new_height, vector<Pixel>(new_width));

    for (int row = 0; row < new_height; ++row)
    {
        for (int col = 0; col < new_width; ++col)
        {
            // Calculate the corresponding pixel in the original image
            int orig_row = floor(row / static_cast<double>(y_scale));
            int orig_col = floor(col / static_cast<double>(x_scale));

            // Set the pixel in the new image
            new_image[row][col] = image[orig_row][orig_col];
        }
    }

    return new_image;
}

// Process 7 - Convert image to high contrast (black and white only)
vector<vector<Pixel>> process_7(const vector<vector<Pixel>> &image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel>(num_columns));

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            const Pixel &p = image[row][col];
            int red_value = p.red;
            int green_value = p.green;
            int blue_value = p.blue;

            // Calculate Gray Value
            int gray_value = (red_value + green_value + blue_value) / 3;

            int new_red, new_green, new_blue;

            if (gray_value >= 255 / 2)
            {
                new_red = 255;
                new_green = 255;
                new_blue = 255;
            }
            else
            {
                new_red = 0;
                new_green = 0;
                new_blue = 0;
            }

            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }

    return new_image;
}

// Process 8 - Lightens image by a scaling factor
vector<vector<Pixel>> process_8(const vector<vector<Pixel>> &image, double scaling_factor)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel>(num_columns));

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            const Pixel &p = image[row][col];
            int red_value = p.red;
            int green_value = p.green;
            int blue_value = p.blue;

            int new_red = static_cast<int>(255 - (255 - red_value) * scaling_factor);
            int new_green = static_cast<int>(255 - (255 - green_value) * scaling_factor);
            int new_blue = static_cast<int>(255 - (255 - blue_value) * scaling_factor);

            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }

    return new_image;
}

// Process 9 - Darkens image by a scaling factor
vector<vector<Pixel>> process_9(const vector<vector<Pixel>> &image, double scaling_factor)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel>(num_columns));

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            const Pixel &p = image[row][col];
            int red_value = p.red;
            int green_value = p.green;
            int blue_value = p.blue;

            int new_red = static_cast<int>(red_value * scaling_factor);
            int new_green = static_cast<int>(green_value * scaling_factor);
            int new_blue = static_cast<int>(blue_value * scaling_factor);

            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }

    return new_image;
}

// Process 10 - Converts image to only black, white, red, blue, and green
vector<vector<Pixel>> process_10(const vector<vector<Pixel>> &image)
{
    int num_rows = image.size();
    int num_columns = image[0].size();

    vector<vector<Pixel>> new_image(num_rows, vector<Pixel>(num_columns));

    for (int row = 0; row < num_rows; ++row)
    {
        for (int col = 0; col < num_columns; ++col)
        {
            const Pixel &p = image[row][col];
            int red_value = p.red;
            int green_value = p.green;
            int blue_value = p.blue;

            int total_color = red_value + green_value + blue_value;
            int new_red, new_green, new_blue;

            // Based on total color value, default to black or white
            new_red = new_green = new_blue = (total_color >= 550) ? 255 : 0;

            // Adjust for dominant color
            if (total_color > 150 && total_color < 550)
            {
                int max_color = max({red_value, green_value, blue_value});
                new_red = (max_color == red_value) ? 255 : 0;
                new_green = (max_color == green_value) ? 255 : 0;
                new_blue = (max_color == blue_value) ? 255 : 0;
            }

            new_image[row][col].red = new_red;
            new_image[row][col].green = new_green;
            new_image[row][col].blue = new_blue;
        }
    }
    return new_image;
}

int main()
{
    string bmpFilename;
    vector<vector<Pixel>> image;
    bool isImageLoaded = false;

    cout << "CSPB 1300 Image Processing Application" << endl;
    bmpFilename = getValidBMPFilename();

    // Condition to exit application
    if (bmpFilename == "")
    {
        return 1;
    }

    // File check
    if (!bmpFilename.empty())
    {
        image = read_image(bmpFilename);
        isImageLoaded = !image.empty();

        if (!isImageLoaded)
        {
            cout << "Image has failed to load. Please restart the application." << endl;
            return 1; // Exit application
        }
    }

    // Main menu loop
    while (true)
    {
        cout << "IMAGE PROCESSING MENU \n"
                "\n";

        cout << "0) Load new image \n"
                "1) Vignette \n"
                "2) Clarendon \n"
                "3) Grayscale \n"
                "4) Rotate 90 degress clockwise \n"
                "5) Rotate multiple 90 degrees \n"
                "6) Enlarge image \n"
                "7) High contrast \n"
                "8) Lighten image \n"
                "9) Darken image \n"
                "10) Black, white, red, gree, blue \n"
                "\n"
                "Q) To quit application \n"
                "\n";

        cout << "Enter menu selection: ";
        string selection;
        cin >> selection;

        if (selection == "Q" || selection == "q")
        {
            cout << "The applicaton will now terminate. Goodbye!" << endl;
            break;
        }

        int option;
        try
        {
            option = stoi(selection); // Convert string to integer for switch statement
        }
        catch (const exception &)
        {
            cout << "Invalid input. Please try again. \n"
                 << endl;
            continue;
        }

        switch (option)
        {
        // Load new image
        case 0:
        {
            bmpFilename = getValidBMPFilename(true);
            // Check to see if action was cancelled
            if (cancellationCheck(bmpFilename))
            {
                break;
            }

            // Load image and ensure it loaded correctly
            if (!bmpFilename.empty())
            {
                image = read_image(bmpFilename);
                isImageLoaded = !image.empty();

                if (!isImageLoaded)
                {
                    cout << "Image has failed to load. Please try again." << endl;
                    break;
                }
                break;
            }
        }
        // Vignette
        case 1:
        {
            cout << "Vignette selected" << endl;

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_1(image);

            write_image(outputFilename, newImage);
            cout << "Successfully applied vignette! \n"
                 << endl;
            break; // continue?
        }
        // Clarendon
        case 2:
        {
            cout << "Clarendon selected" << endl;

            // Get scaling factor
            double scaling_factor;
            while (true)
            {
                cout << "Please enter scaling_factor (e.g. 0.3): ";
                cin >> scaling_factor;

                // Ensure scaling factor is input as a number
                if (cin.fail())
                {
                    cin.clear();                                         // Clear error state
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore invalid input
                    cout << "Invalid scaling factor. Please enter a number." << endl;
                }
                else
                {
                    break;
                }
            }

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_2(image, scaling_factor);

            write_image(outputFilename, newImage);
            cout << "Successfully applied clarendon! \n"
                 << endl;
            break;
        }

        // Grayscale
        case 3:
        {
            cout << "Grayscale selected" << endl;

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_3(image);
            write_image(outputFilename, newImage);
            cout << "Successfully applied grayscale! \n"
                 << endl;
            break;
        }

        // Rotate 90 degrees clockwise
        case 4:
        {
            cout << "Rotate 90 degrees selected" << endl;

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_4(image);
            write_image(outputFilename, newImage);
            cout << "Successfully applied 90 degree rotation! \n"
                 << endl;
            break;
        }

        // Multiple 90 degree rotations
        case 5:
        {
            cout << "Rotate multiple 90 degrees selected" << endl;

            int rotations;
            string input;

            /**
             * get the number of rotations
             * check to ensure an integer was inputted
             * prompt for retry if not an integer
             */

            while (true)
            {
                cout << "Enter number of 90 degree rotations: ";
                cin >> rotations;

                // Check if the input is a valid integer
                if (cin.fail())
                {
                    cin.clear();                                         // Clear error state
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore invalid input
                    cout << "Invalid input. Please enter an integer (e.g. 3)." << endl;
                }
                else
                {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear the buffer of newline
                    break;
                }
            }

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_5(image, rotations);
            write_image(outputFilename, newImage);
            cout << "Successfully applied multiple 90 degree rotations! \n"
                 << endl;
            break;
        }

        // Enlarge image
        case 6:
        {
            cout << "Enlarge selected" << endl;

            int x_scale, y_scale;

            // Get x scale
            while (true)
            {
                cout << "Enter x scale: ";
                cin >> x_scale;
                // Check if the input is a valid integer
                if (cin.fail())
                {
                    cin.clear();                                         // Clear error state
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore invalid input
                    cout << "Invalid x scale. Please enter an integer (e.g. 2)." << endl;
                }
                else
                {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear the buffer of newline
                    break;
                }
            }
            // Get y scale
            while (true)
            {
                cout << "Enter y scale: ";
                cin >> y_scale;
                // Check if the input is a valid integer
                if (cin.fail())
                {
                    cin.clear();                                         // Clear error state
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore invalid input
                    cout << "Invalid y scale. Please enter an integer (e.g. 3)." << endl;
                }
                else
                {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear the buffer of newline
                    break;
                }
            }

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_6(image, x_scale, y_scale);
            write_image(outputFilename, newImage);
            cout << "Successfully enlarged! \n"
                 << endl;
            break;
        }

        // High contrast
        case 7:
        {
            cout << "High contrast selected" << endl;

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_7(image);
            write_image(outputFilename, newImage);
            cout << "Successfully applied high contrast! \n"
                 << endl;
            break;
        }

        // Lighten
        case 8:
        {
            cout << "Lighten selected" << endl;
            // Get scaling factor
            double scaling_factor;
            while (true)
            {
                cout << "Please enter scaling_factor (e.g. 0.3): ";
                cin >> scaling_factor;

                // Ensure scaling factor is input as a number
                if (cin.fail())
                {
                    cin.clear();                                         // clear error state
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore invalid input
                    cout << "Invalid scaling factor. Please enter a number." << endl;
                }
                else
                {
                    break;
                }
            }

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_8(image, scaling_factor);
            write_image(outputFilename, newImage);
            cout << "Successfully lightened! \n"
                 << endl;
            break;
        }

        // Darken
        case 9:
        {
            cout << "Darken selected" << endl;
            // Get scaling factor
            double scaling_factor;
            while (true)
            {
                cout << "Please enter scaling_factor (e.g. 0.3): ";
                cin >> scaling_factor;

                // Ensure scaling factor is input as a number
                if (cin.fail())
                {
                    cin.clear();                                         // Clear error state
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Ignore invalid input
                    cout << "Invalid scaling factor. Please enter a number." << endl;
                }
                else
                {
                    break;
                }
            }

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_9(image, scaling_factor);
            write_image(outputFilename, newImage);
            cout << "Successfully darkened! \n"
                 << endl;
            break;
        }
        // Black, white, red, green, blue
        case 10:
        {
            cout << "Black, white, red, green, blue selected" << endl;

            string outputFilename = getValidBMPFilenameOutput();

            if (cancellationCheck(outputFilename))
            {
                break;
            }

            vector<vector<Pixel>> newImage = process_10(image);
            write_image(outputFilename, newImage);
            cout << "Successfully applied black, white, red, green, blue filter! \n"
                 << endl;
            continue;
        }
        // Default case - handels invalid integer inputs
        default:
            cout << "Invalid input. Please try again. \n"
                 << endl;
            break;
        }
    }

    return 0;
}