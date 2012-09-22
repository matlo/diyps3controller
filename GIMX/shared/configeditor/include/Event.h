/*
 Copyright (c) 2011 Mathieu Laurendeau <mat.lau@laposte.net>
 License: GPLv3
 */

#ifndef EVENT_H
#define EVENT_H

#include <string>

using namespace std;

class Event
{
    public:
        Event();
        Event(string type, string id, string threshold, string deadZone, string multiplier, string exponent, string shape, string buffersize, string filter);
        Event(string type, string id, string threshold);
        Event(string type, string id);
        Event(string type, string id, string deadZone, string multiplier, string exponent, string shape, string buffersize, string filter);
        Event(string id);
        virtual ~Event();
        Event(const Event& other);
        Event& operator=(const Event& other);
        string GetType() { return m_Type; }
        void SetType(string val) { m_Type = val; }
        string GetId() { return m_Id; }
        void SetId(string val) { m_Id = val; }
        string GetThreshold() { return m_Threshold; }
        void SetThreshold(string val) { m_Threshold = val; }
        string GetDeadZone() { return m_DeadZone; }
        void SetDeadZone(string val) { m_DeadZone = val; }
        string GetMultiplier() { return m_Multiplier; }
        void SetMultiplier(string val) { m_Multiplier = val; }
        string GetExponent() { return m_Exponent; }
        void SetExponent(string val) { m_Exponent = val; }
        string GetShape() { return m_Shape; }
        void SetShape(string val) { m_Shape = val; }
        string GetBufferSize() { return m_BufferSize; }
        void SetBufferSize(string val) { m_BufferSize = val; }
        string GetFilter() { return m_Filter; }
        void SetFilter(string val) { m_Filter = val; }
    protected:
    private:
        string m_Type;
        string m_Id;
        string m_Threshold;
        string m_DeadZone;
        string m_Multiplier;
        string m_Exponent;
        string m_Shape;
        string m_BufferSize;
        string m_Filter;
};

#endif // EVENT_H
