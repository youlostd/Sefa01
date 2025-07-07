#pragma once

#include "headers_basic.hpp"
[<INCLUDE_NAME_LIST>]

#include <exception>

namespace network
{
#pragma pack(1)
	struct TPacketHeader
	{
		uint16_t header;
		uint32_t size;

		TPacketHeader() :
			header(0),
			size(0)
		{
		}
	};

	template <typename T>
	struct OutputPacket {
		virtual ~OutputPacket() = default;

		TPacketHeader header;
		T data;

		T* operator->() noexcept { return &data; }
		const T* operator->() const noexcept { return &data; }
		T& operator*() noexcept { return data; }
		const T& operator*() const noexcept { return data; }

		uint16_t get_header() const noexcept { return header.header; }
	};
#pragma pack()

[<CLASS_LIST parentClass="OutputPacket" unknownPacketRaise="std::invalid_argument(\"not handled packet type\")" headerVarName="header.header" tabcount=1>]
}
