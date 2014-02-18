/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "util/scheduler.hpp"

#include <boost/test/unit_test.hpp>

namespace ndn {

BOOST_AUTO_TEST_SUITE(TestScheduler)

struct SchedulerFixture
{
  SchedulerFixture()
    : count1(0)
    , count2(0)
    , count3(0)
    , count4(0)
  {
  }
    
  void
  event1()
  {
    BOOST_CHECK_EQUAL(count3, 1);
    ++count1;
  }

  void
  event2()
  {
    ++count2;
  }

  void
  event3()
  {
    BOOST_CHECK_EQUAL(count1, 0);
    ++count3;
  }

  void
  event4()
  {
    ++count4;
  }
  
  int count1;
  int count2;
  int count3;
  int count4;
};

BOOST_FIXTURE_TEST_CASE(Events, SchedulerFixture)
{
  boost::asio::io_service io; 

  Scheduler scheduler(io);
  scheduler.scheduleEvent(time::seconds(0.1), bind(&SchedulerFixture::event1, this));
  
  EventId i = scheduler.scheduleEvent(time::seconds(0.2), bind(&SchedulerFixture::event2, this));
  scheduler.cancelEvent(i);

  scheduler.scheduleEvent(time::seconds(0.05), bind(&SchedulerFixture::event3, this));

  i = scheduler.scheduleEvent(time::seconds(0.01), bind(&SchedulerFixture::event2, this));
  scheduler.cancelEvent(i);

  i = scheduler.schedulePeriodicEvent(time::seconds(0.3), time::seconds(0.1), bind(&SchedulerFixture::event4, this));
  scheduler.scheduleEvent(time::seconds(0.69), bind(&Scheduler::cancelEvent, &scheduler, i));
  
  io.run();

  BOOST_CHECK_EQUAL(count1, 1);
  BOOST_CHECK_EQUAL(count2, 0);
  BOOST_CHECK_EQUAL(count3, 1);
  BOOST_CHECK_EQUAL(count4, 4);
}

BOOST_AUTO_TEST_CASE(CancelEmptyEvent)
{
  boost::asio::io_service io; 
  Scheduler scheduler(io);
  
  EventId i;
  scheduler.cancelEvent(i);
}

struct SelfCancelFixture
{
  SelfCancelFixture()
    : m_scheduler(m_io)
  {
  }

  void
  cancelSelf()
  {
    m_scheduler.cancelEvent(m_selfEventId);
  }

  boost::asio::io_service m_io;
  Scheduler m_scheduler;
  EventId m_selfEventId;
};

BOOST_FIXTURE_TEST_CASE(SelfCancel, SelfCancelFixture)
{
  m_selfEventId = m_scheduler.scheduleEvent(time::seconds(0.1),
                                            bind(&SelfCancelFixture::cancelSelf, this));

  BOOST_REQUIRE_NO_THROW(m_io.run());
}

struct SelfRescheduleFixture
{
  SelfRescheduleFixture()
    : m_scheduler(m_io)
    , m_count(0)
  {
  }

  void
  reschedule()
  {
    EventId eventId = m_scheduler.scheduleEvent(time::seconds(0.1),
                                                  bind(&SelfRescheduleFixture::reschedule, this));
    m_scheduler.cancelEvent(m_selfEventId);
    m_selfEventId = eventId;

    if(m_count < 5)
      m_count++;
    else
      m_scheduler.cancelEvent(m_selfEventId);
  }

  void
  reschedule2()
  {
    m_scheduler.cancelEvent(m_selfEventId);
    
    
    if(m_count < 5)
      {
        m_selfEventId = m_scheduler.scheduleEvent(time::seconds(0.1),
                                                  bind(&SelfRescheduleFixture::reschedule2, this));
        m_count++;
      }
  }

  void
  doNothing()
  {
    m_count++;
  }
  
  void
  reschedule3()
  {
    m_scheduler.cancelEvent(m_selfEventId);
    
    m_scheduler.scheduleEvent(time::seconds(0.1),
                              bind(&SelfRescheduleFixture::doNothing, this));
    m_scheduler.scheduleEvent(time::seconds(0.1),
                              bind(&SelfRescheduleFixture::doNothing, this));
    m_scheduler.scheduleEvent(time::seconds(0.1),
                              bind(&SelfRescheduleFixture::doNothing, this));
    m_scheduler.scheduleEvent(time::seconds(0.1),
                              bind(&SelfRescheduleFixture::doNothing, this));
    m_scheduler.scheduleEvent(time::seconds(0.1),
                              bind(&SelfRescheduleFixture::doNothing, this));
    m_scheduler.scheduleEvent(time::seconds(0.1),
                              bind(&SelfRescheduleFixture::doNothing, this));
  }

  boost::asio::io_service m_io;
  Scheduler m_scheduler;
  EventId m_selfEventId;
  int m_count;
  
};

BOOST_FIXTURE_TEST_CASE(Reschedule, SelfRescheduleFixture)
{
  m_selfEventId = m_scheduler.scheduleEvent(time::seconds(0),
                                            bind(&SelfRescheduleFixture::reschedule, this));

  BOOST_REQUIRE_NO_THROW(m_io.run());

  BOOST_CHECK_EQUAL(m_count, 5);
}

BOOST_FIXTURE_TEST_CASE(Reschedule2, SelfRescheduleFixture)
{
  m_selfEventId = m_scheduler.scheduleEvent(time::seconds(0),
                                            bind(&SelfRescheduleFixture::reschedule2, this));

  BOOST_REQUIRE_NO_THROW(m_io.run());

  BOOST_CHECK_EQUAL(m_count, 5);
}

BOOST_FIXTURE_TEST_CASE(Reschedule3, SelfRescheduleFixture)
{
  m_selfEventId = m_scheduler.scheduleEvent(time::seconds(0),
                                            bind(&SelfRescheduleFixture::reschedule3, this));

  BOOST_REQUIRE_NO_THROW(m_io.run());

  BOOST_CHECK_EQUAL(m_count, 6);
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace ndn