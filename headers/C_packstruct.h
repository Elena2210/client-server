#ifndef C_PACKSTRUCT_H
#define C_PACKSTRUCT_H

#include <cstring>

namespace packC
{

#pragma pack(1)

// Структура параметров пачки
// используется для задания структуры пачки, которая отправляется к клиенту
struct SPack
{
        // переменная 1
        double num_double;

        // Режим работы
        // 0 =   , 1 =   , 2 =    , 3 =
        unsigned char mode;

        // переменная 2
        unsigned int num_int[3];

        // Название
        short description[128];


	// Конструктор по умолчанию
        SPack()
	{
                memset( this, 0, sizeof( SPack ) );
	}
};

#pragma pack(0)

} // namespace pack

#endif // C_PACKSTRUCT_H
