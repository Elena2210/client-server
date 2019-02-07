#ifndef SERVER_CDGRMS_H
#define SERVER_CDGRMS_H

// Общие типы заголовков
#include "pcktypes.h"

// Структура параметров пачки
#include "S_packstruct.h"

namespace serv
{

///////////////////////////////////////////////////////////////////////////////
// Структуры системы работы сервера

// Типы используемых подзаголовков
enum
{
	MagicNone = 0x00,

        // Тест
        MagicTest,

	MagicError
};

#pragma pack(1)

///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Структура-test
struct SConnTest
{
    // тестирование соединения
    unsigned int test;
};

///////////////////////////////////////////////////////////////////////////////

struct MsgTest : BaseMsg< MTypeServ, MagicTest >
{
        // Структура параметров
//        SConnTest params;

        // Структура параметров для формирования пачки
        pack::SPackParam param;

        MsgTest( const pack::SPackParam & p )
                : BaseMsg( ), param( p ) {}

//        struct MsgSetWorkFreq : BaseMsg< MTypeARM, MagicSetWorkFreq >
//        {
//                // Рабочие частоты, МГц
//                params::SWorkFreq freq;

//                MsgSetWorkFreq( const params::SWorkFreq & f,
//                                                ERadar Radar = ERadar::undefined )
//                        : BaseMsg( Radar, EUnit::arm ), freq( f ) {}
//        };
};
///////////////////////////////////////////////////////////////////////////////

#pragma pack()

} // namespace serv

#endif // SERVER_CDGRMS_H
