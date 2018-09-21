#include "file_manage.h"

#include <algorithm>
#include <time.h>

#include "handshake.h"

std::string execute_upload(custom_sock_t s, std::string& params)
{
	std::string res = "Completed";
	std::string file_name = FILE_SHARED_FOLDER + params;
	
	send_recv_payload_t to_recv;
	file_session_t session;
	session.next_part_to_send_recv = 0;
	session.current_operation = file_operation_upload;
	session.handle = fopen(file_name.c_str(), "wb");
	if (!three_way_handshake_up(s, (bool)(session.handle)))
	{
		if(session.handle)
		{
			fclose(session.handle);
		}
		remove(file_name.c_str());

		res = "Bad handshake";
		return append_newline(res);
	}

	std::vector<uint8_t> serialize_buff;
	std::string recv_buff;
	unsigned prev = 101;
	unsigned curr = 101;

	time_t start = time(nullptr);
	do
	{
		UPLOAD_DOWNLOAD_SIZE_TYPE recv_size = 0;
		if (Socket_Recv(s, recv_buff, sizeof(recv_size)) != sizeof(recv_size))
		{
			fprintf(stderr, "Cannot receive packet size\r\n");
			res = "Error receiving size";
			break;
		}
		recv_size = *((UPLOAD_DOWNLOAD_SIZE_TYPE*)recv_buff.data());
		recv_buff.clear();

		if (Socket_Recv(s, recv_buff, recv_size) != recv_size)
		{
			fprintf(stderr, "Cannot receive packet\r\n");
			res = "Error receiving packet";
			break;
		}

		serialize_buff.insert(serialize_buff.end(), recv_buff.begin(), recv_buff.end());
		recv_buff.clear();

		if (!payload_struct_deserialize(to_recv, serialize_buff, recv_size))
		{
			fprintf(stderr, "Cannot deserialize packet (will be %lu)\r\n", (long unsigned)session.next_part_to_send_recv);
			res = "Error deserialize packet";
			break;
		}
		serialize_buff.clear();

		if (!file_write(session, to_recv))
		{
			res = "Error writing";
			break;
		}

		calc_and_print_stat(&prev ,&curr, &to_recv);
	} while (!(to_recv.is_last));
	time_t stop = time(nullptr);
	calc_and_print_res_load(start, stop, &to_recv);

	fclose(session.handle);

	return append_newline(res);
}

std::string execute_download(custom_sock_t s, std::string& params)
{
	std::string res = "Completed";
	std::string file_name = FILE_SHARED_FOLDER + params;
	send_recv_payload_t to_send;
	file_session_t session;
	session.next_part_to_send_recv = 0;
	session.current_operation = file_operation_download;
	session.handle = fopen(file_name.c_str(), "rb");
	if (!three_way_handshake_down(s, (bool)(session.handle)))
	{
		if(session.handle)
		{
			fclose(session.handle);
		}
		res = "Bad handshake";
		return append_newline(res);
	}

	std::vector<uint8_t> send_buff;
	unsigned prev = 101;
	unsigned curr = 101;

	time_t start = time(nullptr);
	do
	{
		if (!file_read(session, to_send))
		{
			res = "Error reading";
			break;
		}

		UPLOAD_DOWNLOAD_SIZE_TYPE send_size = (UPLOAD_DOWNLOAD_SIZE_TYPE)payload_struct_serialize(to_send, send_buff);
		int res = (int)send(s, (const char*)&send_size, sizeof(send_size), 0);
		if (res != sizeof(send_size))
		{
			fprintf(stderr, "Cannot send packet size. Res = %d, errno = %d", res, CUSTOM_SOCK_ERROR_CODE);
			break;
		}
		res = (int)send(s, (const char*)send_buff.data(), send_size, 0);
		if (res != send_size)
		{
			fprintf(stderr, "Cannot send packet size. Res = %d, errno = %d", res, CUSTOM_SOCK_ERROR_CODE);
			break;
		}

		calc_and_print_stat(&prev, &curr, &to_send);
	} while (!(to_send.is_last));
	time_t stop = time(nullptr);
	calc_and_print_res_load(start, stop, &to_send);

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
	static const int64_t kBlockSize = SERIALIZER_MAX_PAYLOAD_SIZE;

	if (fseek(session.handle, 0, SEEK_END))
	{
		fprintf(stderr, "Read: cannot seek the file\n");
		return false;
	}

	const long file_len = ftell(session.handle);
	to_send.sector_amount = ((file_len - 1) / kBlockSize + 1);

	if (fseek(session.handle, (long)(kBlockSize * session.next_part_to_send_recv), SEEK_SET))
	{
		fprintf(stderr, "Read: cannot seek the file\n");
		return false;
	}

	const size_t were_read = fread(buff, sizeof(*buff), kBlockSize, session.handle);
	to_send.payload.clear();
	to_send.payload.reserve(were_read);
	to_send.payload.insert(to_send.payload.end(), buff, buff + were_read);

	to_send.sector_current_num = session.next_part_to_send_recv;
	session.next_part_to_send_recv++;

	to_send.is_last = ((to_send.sector_current_num + 1) >= to_send.sector_amount);

	return true;
}

bool file_write(file_session_t& session, send_recv_payload_t& to_recv)
{
	static const int64_t kBlockSize = SERIALIZER_MAX_PAYLOAD_SIZE;

	if (fseek(session.handle, (long)(kBlockSize * to_recv.sector_current_num), SEEK_SET))
	{
		fprintf(stderr, "Write: cannot seek the file\n");
		return false;
	}

	const size_t were_write = fwrite(to_recv.payload.data(), sizeof(to_recv.payload[0]), 
		to_recv.payload.size(), session.handle);
	if (were_write != to_recv.payload.size())
	{
		fprintf(stderr, "Were written %lu bytes instead of %lu\r\n", were_write, to_recv.payload.size());
		return false;
	}

	session.next_part_to_send_recv++;

	return true;
}

void calc_and_print_stat(unsigned* prev, unsigned* curr, send_recv_payload_t* send_recv_payload)
{
	if (!prev || !curr || !send_recv_payload) return;

	uint64_t amount = send_recv_payload->sector_amount;
	amount = (amount > 0) ? amount : 1;

	*curr = (unsigned)(100 * (send_recv_payload->sector_current_num + 1) / (amount));

	if (*curr != *prev)
	{
		printf("%u%%\r", *curr);
	}
}

void calc_and_print_res_load(time_t start, time_t stop, send_recv_payload_t* send_recv_payload)
{
	if (!send_recv_payload) return;

	unsigned res = 
		send_recv_payload->sector_amount * SERIALIZER_MAX_PAYLOAD_SIZE /
		(stop - start);
	printf("Load speed: %u kB/s\n", (res / 1000));
}
