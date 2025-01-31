class Time {
    private:
        int hour;
        int minute;
    public:
        Time(int hr, int min);
        Time(unsigned long millis, Time initTime);

        void SetHour(int hr);
        int GetHour();
        void SetMinute(int min);
        int GetMinute();

        void SetTimeMillis(unsigned long millis, Time initTime);
        void SetSysTime(unsigned long millis, int currHr, int currMin);

        bool operator==(const Time& rhs) const;
};