#ifndef CLIENT_CDGRMS_H
#define CLIENT_CDGRMS_H

// Общие типы заголовков
#include "pcktypes.h"

// Структура параметров пачки
#include "C_packstruct.h"

namespace cli
{

///////////////////////////////////////////////////////////////////////////////
// Структуры системы работы клиента

// Типы используемых подзаголовков
enum
{
	MagicNone = 0x00,

        // Режим работы с данными
        MagicJobTest,

	MagicError
};

#pragma pack(1)

///////////////////////////////////////////////////////////////////////////////
// Структура-test
//struct SConnTest
//{
//    // тестирование соединения
//    unsigned int test;
//};
///////////////////////////////////////////////////////////////////////////////
// Команда формирования пачки по структуре параметров пачки
struct MsgJobTest : BaseMsg< MTypeCli, MagicJobTest >
{
	// Структура параметров для формирования пачки
        packC::SPack param;

        // Структура параметров
//        SConnTest params;

        MsgJobTest( const packC::SPack & p  )
                : BaseMsg( ), param( p ) {}
};
///////////////////////////////////////////////////////////////////////////////


#pragma pack()

} // namespace cli

#endif // CLIENT_CDGRMS_H
