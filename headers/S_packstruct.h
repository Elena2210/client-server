#ifndef S_PACKSTRUCT_H
#define S_PACKSTRUCT_H

#include <cstring>

namespace pack
{

#pragma pack(1)

// Структура параметров пачки
// используется для задания структуры пачки, которая отправляется к клиенту
struct SPackParam
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
	SPackParam()
	{
		memset( this, 0, sizeof( SPackParam ) );
	}
};

#pragma pack(0)

} // namespace pack

#endif // S_PACKSTRUCT_H
