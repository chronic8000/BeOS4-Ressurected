_initialize_be_()
{
    _init_gen_malloc();
	_init_sh_malloc_g();
	_init_system_time_();
	_init_sbrk_();
	__sinit();
}
