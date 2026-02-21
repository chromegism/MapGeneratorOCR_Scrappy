#pragma once

#ifdef _DEBUG
	#define DEBUG_LOG std::cout
	#define DEBUG_RUN if (true)
	#define DEBUG_ELSE else
#else
	#define DEBUG_LOG if (true) {} else std::cout
	#define DEBUG_RUN if (false)
	#define DEBUG_ELSE else if (false)
#endif