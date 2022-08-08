#include "Shared.h"

char letters[SALT_SIZE] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
                           'o','p','q','r','s','t','u','v','w','x','y','z','1', '2','3',
                           '4' ,'5' ,'6' };

std::string salt() {

    std::string salt = "";

    for (int i = 0; i < SALT_SIZE; i++) {
        salt = salt + letters[rand() % SALT_SIZE];
    }
    
    return salt;

}

std::string SHA256(std::string string) {

    unsigned char digest[SHA256_DIGEST_LENGTH];

    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, string.c_str(), std::strlen(string.c_str()));
    SHA256_Final(digest, &ctx);

    char mdString[SHA256_DIGEST_LENGTH*2+1];
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        std::sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);

    return std::string(mdString);
}