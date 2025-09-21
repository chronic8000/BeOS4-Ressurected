#if DISABLE_BUILDDATES
const char *build_time = "BeIA 1.1";
const char *build_date = "BeIA 1.1";
#else
const char *build_time = __TIME__;
const char *build_date = __DATE__;
#endif
