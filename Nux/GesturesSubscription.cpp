/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, as
 * published by the  Free Software Foundation; either version 2.1 or 3.0
 * of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License along with this program. If not, see <http://www.gnu.org/licenses/>
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 *
 */

#include "GesturesSubscription.h"
#include "Nux.h"
#include "NuxCore/Logger.h"
#include "Nux/GeisAdapter.h"

namespace nux
{

namespace
{
  logging::Logger logger("nux.gestures_subscription");
}

GesturesSubscription::GesturesSubscription(GestureClass gesture_class,
                                           unsigned int num_touches)
  : gesture_class_(gesture_class),
    num_touches_(num_touches),
    sub_(nullptr),
    is_active_(false)
{
  GeisAdapter *geis_adapter = GetWindowThread()->GetGeisAdapter();

  if (geis_adapter->IsInitComplete())
    CreateGeisSubscription();
  else
    geis_adapter->init_complete.connect(
        sigc::mem_fun(this, &GesturesSubscription::CreateGeisSubscription));
}

GesturesSubscription::~GesturesSubscription()
{
  if (sub_)
    geis_subscription_delete(sub_);
}

void GesturesSubscription::Activate()
{
  GeisStatus status = geis_subscription_activate(sub_);
  if (status == GEIS_STATUS_SUCCESS)
  {
    is_active_ = true;
  }
  else
  {
    LOG_ERROR(logger) << "Failed to activate Geis subscription.";
  }
}

void GesturesSubscription::Deactivate()
{
  GeisStatus status = geis_subscription_deactivate(sub_);
  if (status == GEIS_STATUS_SUCCESS)
  {
    is_active_ = false;
  }
  else
  {
    LOG_ERROR(logger) << "Failed to activate Geis subscription.";
  }
}

GeisString GesturesSubscription::MapToGeisGestureClass(GestureClass nux_gesture_class)
{
  switch (nux_gesture_class)
  {
    case DRAG_GESTURE:
      return GEIS_GESTURE_DRAG;
      break;
    case PINCH_GESTURE:
      return GEIS_GESTURE_PINCH;
      break;
    case ROTATE_GESTURE:
      return GEIS_GESTURE_ROTATE;
      break;
    case TAP_GESTURE:
      return GEIS_GESTURE_TAP;
      break;
    default: // TOUCH_GESTURE
      return GEIS_GESTURE_TOUCH;
  }
}


void GesturesSubscription::CreateGeisSubscription()
{
  GeisStatus status = GEIS_STATUS_UNKNOWN_ERROR;
  GeisFilter filter = nullptr;
  Geis geis = GetWindowThread()->GetGeisAdapter()->GetGeisInstance();

  sub_ = geis_subscription_new(geis, "nux", GEIS_SUBSCRIPTION_NONE);
  if (!sub_)
  {
    LOG_ERROR(logger) << "Failed to create Geis subscription.";
    goto cleanup;
  }

  filter = geis_filter_new(geis, "filter");
  if (!filter)
  {
    LOG_ERROR(logger) << "Failed to create Geis filter.";
    goto cleanup;
  }

  status = geis_filter_add_term(filter,
      GEIS_FILTER_CLASS,
      GEIS_CLASS_ATTRIBUTE_NAME, GEIS_FILTER_OP_EQ,
      MapToGeisGestureClass(gesture_class_),
      GEIS_GESTURE_ATTRIBUTE_TOUCHES, GEIS_FILTER_OP_EQ, num_touches_,
      nullptr);
  if (status != GEIS_STATUS_SUCCESS)
  {
    LOG_ERROR(logger) << "Failed to add term to Geis filter.";
    goto cleanup;
  }

  status = geis_filter_add_term(filter,
      GEIS_FILTER_REGION,
      GEIS_REGION_ATTRIBUTE_WINDOWID, GEIS_FILTER_OP_EQ,
      GetWindowThread()->GetGraphicsDisplay().GetWindowHandle(),
      nullptr);
  if (status != GEIS_STATUS_SUCCESS)
  {
    LOG_ERROR(logger) << "Failed to add region term to Geis filter.";
    goto cleanup;
  }

  status = geis_subscription_add_filter(sub_, filter);
  if (status != GEIS_STATUS_SUCCESS)
  {
    LOG_ERROR(logger) << "Failed to add filter to subscription.";
    goto cleanup;
  }
  filter = nullptr; // it now belongs to the subscription

  status = geis_subscription_activate(sub_);
  if (status != GEIS_STATUS_SUCCESS)
  {
    LOG_ERROR(logger) << "Failed to activate Geis subscription.";
    goto cleanup;
  }

  is_active_ = true;

cleanup:
  if (status != GEIS_STATUS_SUCCESS)
  {
    geis_filter_delete(filter);
    geis_subscription_delete(sub_);
    sub_ = nullptr;
  }
}

bool GesturesSubscription::MatchesGesture(const GestureEvent &event) const
{
  if (event.GetTouches().size() != num_touches_)
    return false;

  if ((event.GetGestureClasses() & gesture_class_) == 0)
    return false;

  return true;
}

} // namespace nux
