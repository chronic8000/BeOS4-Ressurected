          __ALLOC_PAUSE;
            continue;
        }
        if (!__test_and_set((unsigned long*)__lock, 1)) {
            // got it!
            // Spinning worked.  Thus we're probably not being scheduled
            // against the other process with which we were contending.
            // Thus it makes sense to spin longer the next time.
            __last_spins = __i;
            __spin_max = __high_spin_max;
            return;
        }
    }
    // We are probably being scheduled against the other process.  Sleep.
    __spin_max = __low_spin_max;
    for (__i = 0 ;; ++__i) {
        struct timespec __ts;
        int __log_nsec = __i + 6;

        if (!__test_and_set((unsigned long *)__lock, 1)) {
            return;
        }
        if (__log_nsec > 27) __log_nsec = 27;
		/* Max sleep is 2**27nsec ~ 60msec      */
        __ts.tv_sec = 0;
        __ts.tv_nsec = 1 << __log_nsec;
        nanosleep(&__ts, 0);
    }
}

template <bool __threads, i