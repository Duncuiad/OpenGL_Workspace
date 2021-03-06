#include <iostream>
#include <string>

// we load the GLM classes used in the application
#include <glm/glm.hpp>

// we include the library for images loading

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image/stb_image_write.h>

const float PI = 3.14159265359;

glm::vec3 RGBToVec3(unsigned char R, unsigned char G, unsigned char B);
glm::vec4 RGBAToVec4(unsigned char R, unsigned char G, unsigned char B, unsigned char A);

void vec3ToRGB(glm::vec3 vector, unsigned char here[3]);
void vec4ToRGBA(glm::vec4 vector, unsigned char here[4]);

float AshikhminPartialPhi(float u, float nU, float nV);
float AshikhminPhi(float u, float nU, float nV);
float AshikhminCosTheta(float v, float exponent, float nU, float nV);

// -------------- GLOBAL VARIABLES -------------- //

// the width and height of the output texture
unsigned int size = 512;

// directional shininess parameters of Ashikhmin-Shirley anisotropic illumination model
float nU = 1.0;
float nV = 1.0;

int main() 
{

    // input management for the parameters nU and nV
    while (std::cout << "Insert value for nU: " && !(std::cin >> nU))
    {
        std::cin.clear(); //clear bad input flag
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //discard input
        std::cout << "Invalid input; please re-enter.\n";
    }
    while (std::cout << "Insert value for nV: " && !(std::cin >> nV))
    {
        std::cin.clear(); //clear bad input flag
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); //discard input
        std::cout << "Invalid input; please re-enter.\n";
    }

    // automated management of the pathname of the output texture, based on nU and nV values
    std::string saveName = "../../textures/halfVectorSampling";
    unsigned int current = 0;
    std::string format = "png";

    std::string strNU = std::to_string(nU);
    strNU.erase(strNU.find_last_not_of('0') + 1, std::string::npos);
    strNU.erase(strNU.find_last_not_of('.') + 1, std::string::npos);
    std::string strNV = std::to_string(nV);
    strNV.erase(strNV.find_last_not_of('0') + 1, std::string::npos);
    strNV.erase(strNV.find_last_not_of('.') + 1, std::string::npos);
    std::string shininess = " [" + strNU + "," + strNV + "]";

    // variables for managing writing of texture
    std::string fullPath = saveName + shininess + "." + format;
    unsigned char rgbaBuffer[4]; // given to Vec4ToRGBA, which stores the output values for each texel in the buffer

    unsigned char* image = new unsigned char[4*size*size]; // output texture buffer

    // check if the file already exists, in which case the program doesn't overwrite it
    while(stbi_info(fullPath.c_str(), NULL, NULL, NULL) != 0) // this file already exists
    {
        current++;

        // incrementally change the name of the file to write onto
        fullPath = saveName + shininess + " " + std::to_string(current) + "." + format;
    }

    // generate the texture
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            // these are the (u,v) coordinates for the output texture
            float u = ((float) i) / ((float) size); // from the left of the image
            float v = ((float) size - j - 1) / ((float) size); // from the bottom of the image

            // mapping of the unit square to (phi, theta) spherical coordinates
            // this is done following Ashikhmin-Shirley importance sampling, and depends on the values of nU and nV
            float phi = AshikhminPhi(u, nU, nV);
            float cosPhi = glm::cos(phi);
            float sinPhi = glm::sin(phi);
            float ashikhminExponent = nU * cosPhi * cosPhi + nV * sinPhi * sinPhi;

            float cosTheta = AshikhminCosTheta(v, ashikhminExponent, nU, nV);
            float sinTheta = glm::sqrt(1.0 - cosTheta * cosTheta);

            // from spherical (2D) to cartesian (3D) coordinates
            float x = sinTheta * cosPhi;
            float y = sinTheta * sinPhi;
            float z = cosTheta;

            // calculate the probability density for the mapped vector, which is to be stored in the alpha channel of the texture
            float pdf = glm::pow(cosTheta, ashikhminExponent);
            // I don't multiply for the normalization constant: the result might be greater than 1, making it impossible to save in a texture
            // pdf *= glm::sqrt((nU + 1.0) * (nV + 1.0)) / 2.0 / PI;

            // save the vector onto the texture
            glm::vec4 halfVector(x, y, z, pdf);

            vec4ToRGBA(halfVector, rgbaBuffer);

            unsigned int bufferPosition = 4*(size*j + i);

            image[bufferPosition] = rgbaBuffer[0];
            image[bufferPosition + 1] = rgbaBuffer[1];
            image[bufferPosition + 2] = rgbaBuffer[2];
            image[bufferPosition + 3] = rgbaBuffer[3] == 0 ? 1 : rgbaBuffer[3]; // this is the pdf; I make sure I don't end up dividing by zero later
        }            
    }

    // save the texture
    stbi_write_png(fullPath.c_str(), size, size, STBI_rgb_alpha, image, size * STBI_rgb_alpha);

    // free space
    delete[] image;

    return 0;
}

glm::vec3 RGBToVec3(unsigned char R, unsigned char G, unsigned char B)
{
    float x = ((float) R) / 127.5 - 1.0;
    float y = ((float) G) / 127.5 - 1.0;
    float z = ((float) B) / 127.5 - 1.0;
    return glm::vec3(x, y, z);
}

glm::vec4 RGBAToVec4(unsigned char R, unsigned char G, unsigned char B, unsigned char A)
{
    float x = ((float) R) / 127.5 - 1.0;
    float y = ((float) G) / 127.5 - 1.0;
    float z = ((float) B) / 127.5 - 1.0;
    float w = ((float) A) / 127.5 - 1.0;
    return glm::vec4(x, y, z, w);
}

void vec3ToRGB(glm::vec3 vector, unsigned char here[3])
{
    here[0] = (unsigned char) (floorf(vector.x * 127.5 + 127.5));
    here[1] = (unsigned char) (floorf(vector.y * 127.5 + 127.5));
    here[2] = (unsigned char) (floorf(vector.z * 127.5 + 127.5));
}

void vec4ToRGBA(glm::vec4 vector, unsigned char here[4])
{
    here[0] = (unsigned char) (floorf(vector.x * 127.5 + 127.5));
    here[1] = (unsigned char) (floorf(vector.y * 127.5 + 127.5));
    here[2] = (unsigned char) (floorf(vector.z * 127.5 + 127.5));
    here[3] = (unsigned char) (floorf(vector.w * 127.5 + 127.5));
}

// calculation of phi in the first quadrant, as per Ashikhmin-Shirley importance sampling
float AshikhminPartialPhi(float x, float nU, float nV)
{
    if (x == 1.0) // the tangent explodes to infinity
        return PI / 2.0;

    float coeff = glm::sqrt((nU + 1.0) / (nV + 1.0));
    float tang = glm::tan(PI * x / 2.0);
    return glm::atan(coeff * tang);
}

// Ashikhmin-Shirley only generates phi in the first quadrant
// I use this function to map and flip to the other quadrants, gluing along the axes in the right way
float AshikhminPhi(float u, float nU, float nV)
{
    float phi = 0.0;

    // first quadrant
    if (u <= 0.25)
    {
        phi = AshikhminPartialPhi(4.0 * u, nU, nV);
    }

    // second quadrant
    else if (u < 0.5)
    {
        phi = PI - AshikhminPartialPhi(2.0 - 4.0 * u, nU, nV);
    }

    // third quadrant
    else if (u < 0.75)
    {
        phi = PI + AshikhminPartialPhi(4.0 * u - 2.0, nU, nV);
    }

    // fourth quadrant
    else if (u < 1.0)
    {
        phi = 2.0 * PI - AshikhminPartialPhi(4.0 * (1.0 - u), nU, nV);
    }

    // notice u == 1.0 would be mapped by the last "else if" to 2PI == 0.0, which it is anyway, skipping all of the checks
    return phi;
}

// calculation of cosTheta, as per Ashikhmin-Shirley importance sampling
float AshikhminCosTheta(float v, float exponent, float nU, float nV)
{
    float thetaExponent = 1.0 / (exponent + 1.0);
    return glm::pow(1.0 - v, thetaExponent);
}