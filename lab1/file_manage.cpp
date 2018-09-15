#include "file_manage.h"

#include <algorithm>

std::string execute_upload(custom_sock_t s, std::string& params)
{
	//TODO:
	std::string temp("TODO: upload");
	return append_newline(temp);
}

std::string execute_download(custom_sock_t s, std::string& params)
{
	std::string res = "Completed";
	std::string file_name = FILE_SHARED_FOLDER + params;
	send_recv_payload_t to_send;
	file_session_t session;
	session.next_part_to_send = 0;
	session.current_operation = file_operation_upload;
	session.handle = fopen(file_name.c_str(), "rb");
	if (!(session.handle))
	{
		res = "Cannot open input file";
		return append_newline(res);
	}

	do
	{
		if (!file_read(session, to_send))
		{
			res = "Error reading";
			break;
		}
		//TODO:
	} while (!(to_send.is_last));

	fclose(session.handle);

	return append_newline(res);
}

std::string append_newline(std::string& s)
{
	return (s + SOCKET_COMMAND_DELIMITER_0 + SOCKET_COMMAND_DELIMITER_1);
}

std::string execute_continue_upload(custom_sock_t s, std::string& params)
{
	//TODO:
	std::string temp("TODO: upload continue");
	return append_newline(temp);
}
std::string execute_continue_download(custom_sock_t s, std::string& params)
{
	//TODO:
	std::string temp("TODO: download continue");
	return append_newline(temp);
}

bool file_read(file_session_t& session, send_recv_payload_t& to_send)
{
	uint8_t buff[SERIALIZER_MAX_PAYLOAD_SIZE];
	static const uint64_t kBlockSize = SERIALIZER_MAX_PAYLOAD_SIZE;

	if (fseek(session.handle, 0, SEEK_END))
	{
		fprintf(stderr, "Read: cannot seek the file\n");
		return false;
	}

	const long file_len = ftell(session.handle);
	to_send.sector_amount = ((file_len - 1) / kBlockSize + 1);
	printf("Out file len: %ld bytes\r\n", file_len);

	if (fseek(session.handle, (long)(kBlockSize * session.next_part_to_send), SEEK_SET))
	{
		fprintf(stderr, "Read: cannot seek the file\n");
		return false;
	}

	const size_t were_read = fread(buff, sizeof(*buff), kBlockSize, session.handle);
	to_send.payload.clear();
	to_send.payload.reserve(were_read);
	to_send.payload.insert(to_send.payload.end(), buff, buff + were_read);

	to_send.sector_current_num = session.next_part_to_send;
	session.next_part_to_send++;

	to_send.is_last = ((to_send.sector_current_num + 1) >= to_send.sector_amount);

	return true;
}
