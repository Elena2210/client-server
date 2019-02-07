#ifndef PCKTYPES_H
#define PCKTYPES_H

/*
 * Заголовок пакета - это первый uint16 в передаваемой структуре данных.
 * Кроме заголовка пакет может содержать подзаголовок в зависимости от типа.
 * КБ Радар
 */

#include <cstdint>
#include <QDateTime>

enum EMType
{
	// Резерв
	MTypeNON = 0x00,

        // Система соединений                                   //1
        MTypeCli,

        // Система соединений                                   //2
        MTypeServ,

//	// Система АДУ						// 1
//	MTypeADU,

//	// Система ПОИ						// 2
//	MTypePOI,

//	// Система ВОИ						// 3
//	MTypeVOI,


	// Резерв
	MTypeERR
};

#pragma pack(1)

///////////////////////////////////////////////////////////////////////////////
// Шаблон заголовка пакета
template <unsigned char MType = 0, unsigned char Magic = 0>
struct BaseMsg
{
	const unsigned char hdr = MType;
	const unsigned char magic = Magic;

	// метка времени
	// the number of milliseconds that have passed since 1970-01-01T00:00:00.000
	// Coordinated Universal Time (Qt::UTC).
	long long reg_time;

	// Уникальный идентификатор пакета
	unsigned int packet_id;

        BaseMsg( long long t = QDateTime::currentMSecsSinceEpoch() )
                : reg_time( t )
	{
		static unsigned int unique_id = 0;
		packet_id = ++unique_id;
	}

};

///////////////////////////////////////////////////////////////////////////////
// Конвертация структуры в QByteArray
template < class T >
QByteArray toQByteArray( const T & msg )
{
	return QByteArray( ( char * )&msg, sizeof( msg ) );
}

#pragma pack()

#endif // PCKTYPES_H
