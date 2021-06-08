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

std::string hash(std::string pwd, std::string salt) {

    std::string std_s = pwd + salt;
    int len = std_s.length();

    std::cout << "YEET" << std::endl;
    std::cout << std_s << std::endl;
    std::cout << len << std::endl;

    char *input = &std_s[0];

    unsigned char md[SHA256_DIGEST_LENGTH]; // 32 bytes
    if(!SHA256(input, len, md))
    {
        std::cout << "Hashing failed" << std::endl;
    }
    std::cout << md << std::endl;

    std::string result(reinterpret_cast< char const* >(md), SHA256_DIGEST_LENGTH + 1);

    return result;
// �sgK���?�=����A�C|P&ň��ȤW�
// �sgK���?�=����A�C|P&ň��ȤW�
// �sgK���?�=����A�C|P&ň��ȤW�

}

bool SHA256(void* input, unsigned long length, unsigned char* md)
{
    SHA256_CTX context;
    if(!SHA256_Init(&context))
        return false;

    if(!SHA256_Update(&context, (unsigned char*)input, length))
        return false;

    if(!SHA256_Final(md, &context))
        return false;

    return true;
}