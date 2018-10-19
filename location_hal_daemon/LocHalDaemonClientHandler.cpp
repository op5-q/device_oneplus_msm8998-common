/* Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cinttypes>
#include <gps_extended_c.h>
#include <LocationApiMsg.h>
#include <LocHalDaemonClientHandler.h>
#include <LocationApiService.h>

/******************************************************************************
LocHalDaemonClientHandler - updateSubscriptionMask
******************************************************************************/
void LocHalDaemonClientHandler::updateSubscription(uint32_t mask) {

    // update my subscription mask
    mSubscriptionMask = mask;

    // set callback functions for Location API
    mCallbacks.size = sizeof(mCallbacks);

    // mandatory callback
    mCallbacks.capabilitiesCb = [this](LocationCapabilitiesMask mask) {
        onCapabilitiesCallback(mask);
    };
    mCallbacks.responseCb = [this](LocationError err, uint32_t id) {
        onResponseCb(err, id);
    };
    mCallbacks.collectiveResponseCb =
            [this](size_t count, LocationError* errs, uint32_t* ids) {
        onCollectiveResponseCallback(count, errs, ids);
    };

    // update optional callback - following four callbacks can be controlable
    // tracking
    mCallbacks.trackingCb = [this](Location location) {
        onTrackingCb(location);
    };

    // batching
    if (mSubscriptionMask & E_LOC_CB_BATCHING_BIT) {
        mCallbacks.batchingCb = [this](size_t count, Location* location,
                BatchingOptions batchingOptions) {
            onBatchingCb(count, location, batchingOptions);
        };
    } else {
        mCallbacks.batchingCb = nullptr;
    }
    // batchingStatus
    if (mSubscriptionMask & E_LOC_CB_BATCHING_STATUS_BIT) {
        mCallbacks.batchingStatusCb = [this](BatchingStatusInfo batchingStatus,
                std::list<uint32_t>& listOfCompletedTrips) {
            onBatchingStatusCb(batchingStatus, listOfCompletedTrips);
        };
    } else {
        mCallbacks.batchingStatusCb = nullptr;
    }

    // location info
    if (mSubscriptionMask & E_LOC_CB_GNSS_LOCATION_INFO_BIT) {
        mCallbacks.gnssLocationInfoCb = [this](GnssLocationInfoNotification notification) {
            onGnssLocationInfoCb(notification);
        };
    } else {
        mCallbacks.gnssLocationInfoCb = nullptr;
    }

    // sv info
    if (mSubscriptionMask & E_LOC_CB_GNSS_SV_BIT) {
        mCallbacks.gnssSvCb = [this](GnssSvNotification notification) {
            onGnssSvCb(notification);
        };
    } else {
        mCallbacks.gnssSvCb = nullptr;
    }

    // nmea
    if (mSubscriptionMask & E_LOC_CB_GNSS_NMEA_BIT) {
        mCallbacks.gnssNmeaCb = [this](GnssNmeaNotification notification) {
            onGnssNmeaCb(notification);
        };
    } else {
        mCallbacks.gnssNmeaCb = nullptr;
    }

    // data
    if (mSubscriptionMask & E_LOC_CB_GNSS_DATA_BIT) {
        mCallbacks.gnssDataCb = [this](GnssDataNotification notification) {
            onGnssDataCb(notification);
        };
    } else {
        mCallbacks.gnssDataCb = nullptr;
    }

    // system info
    if (mSubscriptionMask & E_LOC_CB_SYSTEM_INFO_BIT) {
        mCallbacks.locationSystemInfoCb = [this](LocationSystemInfo notification) {
            onLocationSystemInfoCb(notification);
        };
    } else {
        mCallbacks.locationSystemInfoCb = nullptr;
    }

    // following callbacks are not supported
    mCallbacks.gnssMeasurementsCb = nullptr;
    mCallbacks.gnssNiCb = nullptr;
    mCallbacks.geofenceBreachCb = nullptr;
    mCallbacks.geofenceStatusCb = nullptr;

    // call location API if already created
    if (mLocationApi) {
        LOC_LOGd("--> updateCallbacks mask=0x%x", mask);
        mLocationApi->updateCallbacks(mCallbacks);
    }
}

uint32_t LocHalDaemonClientHandler::startTracking() {
    if (mSessionId == 0) {
        mSessionId = mLocationApi->startTracking(mOptions);
    }
    return mSessionId;
}

uint32_t LocHalDaemonClientHandler::startTracking(uint32_t minDistance, uint32_t minInterval) {
    if (mSessionId == 0) {
        // update option
        mOptions.size = sizeof(mOptions);
        mOptions.minDistance = minDistance;
        mOptions.minInterval = minInterval;
        mOptions.mode = GNSS_SUPL_MODE_STANDALONE;
        mSessionId = mLocationApi->startTracking(mOptions);
    }
    return mSessionId;
}

void LocHalDaemonClientHandler::unsubscribeLocationSessionCb() {

    uint32_t subscriptionMask = mSubscriptionMask;

    subscriptionMask &= ~LOCATION_SESSON_ALL_INFO_MASK;
    updateSubscription(subscriptionMask);
}

void LocHalDaemonClientHandler::stopTracking() {
    if (mSessionId != 0) {
        mLocationApi->stopTracking(mSessionId);
        mSessionId = 0;
    }
}

void LocHalDaemonClientHandler::updateTrackingOptions(uint32_t minDistance, uint32_t minInterval) {
    if (mSessionId != 0) {
        // update option
        mOptions.size = sizeof(mOptions);
        mOptions.minDistance = minDistance;
        mOptions.minInterval = minInterval;
        mOptions.mode = GNSS_SUPL_MODE_STANDALONE;
        mLocationApi->updateTrackingOptions(mSessionId, mOptions);
    }
}

uint32_t LocHalDaemonClientHandler::startBatching(uint32_t minInterval, uint32_t minDistance,
        BatchingMode batchMode) {
    if (mBatchingId == 0) {
        // update option
        LocationOptions locOption = {};
        locOption.size = sizeof(locOption);
        locOption.minInterval = minInterval;
        locOption.minDistance = minDistance;
        locOption.mode = GNSS_SUPL_MODE_STANDALONE;
        mBatchOptions.size = sizeof(mBatchOptions);
        mBatchOptions.batchingMode = batchMode;
        mBatchOptions.setLocationOptions(locOption);

        mBatchingId = mLocationApi->startBatching(mBatchOptions);
    }
    return mBatchingId;

}

void LocHalDaemonClientHandler::stopBatching() {
    if (mBatchingId != 0) {
        mLocationApi->stopBatching(mBatchingId);
        mBatchingId = 0;
    }
}

void LocHalDaemonClientHandler::updateBatchingOptions(uint32_t minInterval, uint32_t minDistance,
        BatchingMode batchMode) {
    if (mBatchingId != 0) {
        // update option
        LocationOptions locOption = {};
        locOption.size = sizeof(locOption);
        locOption.minInterval = minInterval;
        locOption.minDistance = minDistance;
        locOption.mode = GNSS_SUPL_MODE_STANDALONE;
        mBatchOptions.size = sizeof(mBatchOptions);
        mBatchOptions.batchingMode = batchMode;
        mBatchOptions.setLocationOptions(locOption);

        mLocationApi->updateBatchingOptions(mBatchingId, mBatchOptions);
    }
}


/******************************************************************************
LocHalDaemonClientHandler - Location API response callback functions
******************************************************************************/
void LocHalDaemonClientHandler::onResponseCb(LocationError err, uint32_t id) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onResponseCb err=%u id=%u", err, id);

    ELocMsgID pendingMsgId = E_LOCAPI_UNDEFINED_MSG_ID;
    if (!mPendingMessages.empty()) {
        pendingMsgId = mPendingMessages.front();
        mPendingMessages.pop();
    }

    int rc = 0;
    // send corresponding indication message if pending
    switch (pendingMsgId) {
        case E_LOCAPI_START_TRACKING_MSG_ID: {
            LOC_LOGd("<-- start resp err=%u id=%u pending=%u", err, id, pendingMsgId);
            LocAPIGenericRespMsg msg(SERVICE_NAME, E_LOCAPI_START_TRACKING_MSG_ID, err);
            rc = sendMessage(msg);
            break;
        }
        case E_LOCAPI_STOP_TRACKING_MSG_ID: {
            LOC_LOGd("<-- stop resp err=%u id=%u pending=%u", err, id, pendingMsgId);
            LocAPIGenericRespMsg msg(SERVICE_NAME, E_LOCAPI_STOP_TRACKING_MSG_ID, err);
            rc = sendMessage(msg);
            break;
        }
        case E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID: {
            LOC_LOGd("<-- update resp err=%u id=%u pending=%u", err, id, pendingMsgId);
            LocAPIGenericRespMsg msg(SERVICE_NAME, E_LOCAPI_UPDATE_TRACKING_OPTIONS_MSG_ID, err);
            rc = sendMessage(msg);
            break;
        }
        case E_LOCAPI_START_BATCHING_MSG_ID : {
            LOC_LOGd("<-- start batching resp err=%u id=%u pending=%u", err, id, pendingMsgId);
            LocAPIGenericRespMsg msg(SERVICE_NAME, E_LOCAPI_START_BATCHING_MSG_ID , err);
            rc = sendMessage(msg);
            break;
        }
        case E_LOCAPI_STOP_BATCHING_MSG_ID: {
            LOC_LOGd("<-- stop batching resp err=%u id=%u pending=%u", err, id, pendingMsgId);
            LocAPIGenericRespMsg msg(SERVICE_NAME, E_LOCAPI_STOP_BATCHING_MSG_ID, err);
            rc = sendMessage(msg);
            break;
        }
        case E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID: {
            LOC_LOGd("<-- update batching options resp err=%u id=%u pending=%u", err, id,
                    pendingMsgId);
            LocAPIGenericRespMsg msg(SERVICE_NAME, E_LOCAPI_UPDATE_BATCHING_OPTIONS_MSG_ID, err);
            rc = sendMessage(msg);
            break;
        }
        default: {
            LOC_LOGe("no pending message for %s", mName.c_str());
            return;
        }
    }

    // purge this client if failed
    if (!rc) {
        LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
        mService->deleteClientbyName(mName);
    }
}

void LocHalDaemonClientHandler::onCollectiveResponseCallback(
        size_t count, LocationError *errs, uint32_t *ids) {
    LOC_LOGd("--< onCollectiveResponseCallback");
}

/******************************************************************************
LocHalDaemonClientHandler - Location API callback functions
******************************************************************************/
void LocHalDaemonClientHandler::onCapabilitiesCallback(LocationCapabilitiesMask mask) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onCapabilitiesCallback=0x%x", mask);

    // broadcast
    LocAPICapabilitiesIndMsg msg(SERVICE_NAME, mask);
    if (mask != mCapabilityMask) {
        LOC_LOGd("mask old=0x%x new=0x%x", mCapabilityMask, mask);
        mCapabilityMask = mask;
        int rc = sendMessage(msg);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }
}

void LocHalDaemonClientHandler::onTrackingCb(Location location) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onTrackingCb");

    // broadcast
    LocAPILocationIndMsg msg(SERVICE_NAME, location);
    if (mSubscriptionMask & E_LOC_CB_TRACKING_BIT) {
        int rc = sendMessage(msg);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }
}

void LocHalDaemonClientHandler::onBatchingCb(size_t count, Location* location,
        BatchingOptions batchOptions) {
    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onBatchingCb");

    if (0 == count) {
        return;
    }
    // serialize locations in batch into ipc message payload
    size_t msglen = sizeof(LocAPIBatchingIndMsg) + sizeof(Location) * (count - 1);
    uint8_t *msg = new(std::nothrow) uint8_t[msglen];
    if (nullptr == msg) {
        return;
    }
    memset(msg, 0, msglen);
    LocAPIBatchingIndMsg *pmsg = reinterpret_cast<LocAPIBatchingIndMsg*>(msg);
    strlcpy(pmsg->mSocketName, SERVICE_NAME, MAX_SOCKET_PATHNAME_LENGTH);
    pmsg->msgId = E_LOCAPI_BATCHING_MSG_ID;
    pmsg->batchNotification.size = msglen;
    pmsg->batchNotification.count = count;
    pmsg->batchNotification.status = BATCHING_STATUS_POSITION_AVAILABE;
    memcpy(&(pmsg->batchNotification.location[0]), location, count * sizeof(Location));

    if (mSubscriptionMask & E_LOC_CB_BATCHING_BIT) {
        int rc = sendMessage(msg, msglen);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }

    delete[] msg;
}

void LocHalDaemonClientHandler::onBatchingStatusCb(BatchingStatusInfo batchingStatus,
                std::list<uint32_t>& listOfCompletedTrips) {
    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onBatchingStatusCb");
    if (BATCHING_MODE_TRIP == mBatchingMode &&
                BATCHING_STATUS_TRIP_COMPLETED == batchingStatus.batchingStatus) {
        // For trip batching, notify client to stop session when BATCHING_STATUS_TRIP_COMPLETED
        LocAPIBatchNotification batchNotif = {};
        batchNotif.status = BATCHING_STATUS_TRIP_COMPLETED;
        LocAPIBatchingIndMsg msg(SERVICE_NAME, batchNotif);
        if (mSubscriptionMask & E_LOC_CB_BATCHING_STATUS_BIT) {
            int rc = sendMessage(msg);
            // purge this client if failed
            if (!rc) {
                LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
                mService->deleteClientbyName(mName);
            }
        }

    }
}

void LocHalDaemonClientHandler::onGnssLocationInfoCb(GnssLocationInfoNotification notification) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssLocationInfoCb");

    LocAPILocationInfoIndMsg msg(SERVICE_NAME, notification);
    if (mSubscriptionMask & E_LOC_CB_GNSS_LOCATION_INFO_BIT) {
        int rc = sendMessage(msg);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }
}

void LocHalDaemonClientHandler::onGnssNiCb(uint32_t id, GnssNiNotification gnssNiNotification) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssNiCb");
}

void LocHalDaemonClientHandler::onGnssSvCb(GnssSvNotification notification) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssSvCb");

    // broadcast
    LocAPISatelliteVehicleIndMsg msg(SERVICE_NAME, notification);
    if (mSubscriptionMask & E_LOC_CB_GNSS_SV_BIT) {
        int rc = sendMessage(msg);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }
}

void LocHalDaemonClientHandler::onGnssNmeaCb(GnssNmeaNotification notification) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssNmeaCb[%s] t=%" PRIu64" l=%zu nmea=%s",
            mName.c_str(),
            notification.timestamp,
            notification.length,
            notification.nmea);

    // serialize nmea string into ipc message payload
    size_t msglen = sizeof(LocAPINmeaIndMsg) + notification.length;
    uint8_t *msg = new(std::nothrow) uint8_t[msglen];
    if (nullptr == msg) {
        return;
    }
    memset(msg, 0, msglen);
    LocAPINmeaIndMsg *pmsg = reinterpret_cast<LocAPINmeaIndMsg*>(msg);
    strlcpy(pmsg->mSocketName, SERVICE_NAME, MAX_SOCKET_PATHNAME_LENGTH);
    pmsg->msgId = E_LOCAPI_NMEA_MSG_ID;
    pmsg->gnssNmeaNotification.size = msglen;
    pmsg->gnssNmeaNotification.timestamp = notification.timestamp;
    pmsg->gnssNmeaNotification.length = notification.length;
    memcpy(&(pmsg->gnssNmeaNotification.nmea[0]), notification.nmea, notification.length);

    if (mSubscriptionMask & E_LOC_CB_GNSS_NMEA_BIT) {
        int rc = sendMessage(msg, msglen);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }

    delete[] msg;
}

void LocHalDaemonClientHandler::onGnssDataCb(GnssDataNotification notification) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssDataCb");
    for (int sig = 0; sig < GNSS_LOC_MAX_NUMBER_OF_SIGNAL_TYPES; sig++) {
        if (GNSS_LOC_DATA_JAMMER_IND_BIT ==
            (notification.gnssDataMask[sig] & GNSS_LOC_DATA_JAMMER_IND_BIT)) {
            LOC_LOGv("jammerInd[%d]=%f", sig, notification.jammerInd[sig]);
        }
        if (GNSS_LOC_DATA_AGC_BIT ==
            (notification.gnssDataMask[sig] & GNSS_LOC_DATA_AGC_BIT)) {
            LOC_LOGv("agc[%d]=%f", sig, notification.agc[sig]);
        }
    }

    LocAPIDataIndMsg msg(SERVICE_NAME, notification);
    if (mSubscriptionMask & E_LOC_CB_GNSS_DATA_BIT) {
        LOC_LOGv("Sending data message");
        int rc = sendMessage(msg);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }
}

void LocHalDaemonClientHandler::onGnssMeasurementsCb(GnssMeasurementsNotification notification) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onGnssMeasurementsCb");
}

void LocHalDaemonClientHandler::onLocationSystemInfoCb(LocationSystemInfo notification) {

    std::lock_guard<std::mutex> lock(LocationApiService::mMutex);
    LOC_LOGd("--< onLocationSystemInfoCb");

    LocAPILocationSystemInfoIndMsg msg(SERVICE_NAME, notification);
    if (mSubscriptionMask & E_LOC_CB_SYSTEM_INFO_BIT) {
        LOC_LOGv("Sending location system info message");
        int rc = sendMessage(msg);
        // purge this client if failed
        if (!rc) {
            LOC_LOGe("failed rc=%d purging client=%s", rc, mName.c_str());
            mService->deleteClientbyName(mName);
        }
    }
}

/******************************************************************************
LocHalDaemonClientHandler - Engine info related functionality
******************************************************************************/
// called to deliver GNSS energy consumed info to the requesting client
// as this is single shot request, the corresponding mask will be cleared
// as well
void LocHalDaemonClientHandler::onGnssEnergyConsumedInfoAvailable(
   LocAPIGnssEnergyConsumedIndMsg &msg) {

    if (mEngineInfoRequestMask & E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT) {
        sendMessage(msg);
        mEngineInfoRequestMask &= ~E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT;
    }
}

// return true if the client has pending request to retrieve
// GNSS energy consumed
bool LocHalDaemonClientHandler::hasPendingEngineInfoRequest(uint32_t mask) {
    if (mEngineInfoRequestMask & E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT) {
        return true;
    } else {
        return false;
    }
}

// set up the bit to indicating the engine info request
// is pending.
void LocHalDaemonClientHandler::addEngineInfoRequst(uint32_t mask) {
    mEngineInfoRequestMask |= E_ENGINE_INFO_CB_GNSS_ENERGY_CONSUMED_BIT;
}

