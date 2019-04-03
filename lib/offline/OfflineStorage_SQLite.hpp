#include "mat/config.h"
#ifdef HAVE_MAT_STORAGE
// Copyright (c) Microsoft. All rights reserved.

#pragma once
#include "pal/PAL.hpp"
#include "IOfflineStorage.hpp"

#include "api/IRuntimeConfig.hpp"

#include "ILogManager.hpp"

#include <memory>
#include <atomic>
#include <mutex>

#define ENABLE_LOCKING      // Enable DB locking for flush

namespace ARIASDK_NS_BEGIN {

    class SqliteDB;

    class OfflineStorage_SQLite : public IOfflineStorage
    {
    public:
        OfflineStorage_SQLite(ILogManager& logManager, IRuntimeConfig& runtimeConfig, bool inMemory = false);

        virtual ~OfflineStorage_SQLite() override;
        virtual void Initialize(IOfflineStorageObserver& observer) override;
        virtual void Shutdown() override;
        virtual void Flush() override {};
        virtual void Execute(std::string command);
        virtual bool StoreRecord(StorageRecord const& record) override;
        virtual bool GetAndReserveRecords(std::function<bool(StorageRecord&&)> const& consumer, unsigned leaseTimeMs, EventLatency minLatency = EventLatency_Normal, unsigned maxCount = 0) override;
        virtual bool IsLastReadFromMemory() override;
        virtual unsigned LastReadRecordCount() override;

        virtual void DeleteRecords(const std::map<std::string, std::string> & whereFilter) override;
        virtual void DeleteRecords(std::vector<StorageRecordId> const& ids, HttpHeaders headers, bool& fromMemory) override;
        virtual void ReleaseRecords(std::vector<StorageRecordId> const& ids, bool incrementRetryCount, HttpHeaders headers, bool& fromMemory) override;

        virtual bool StoreSetting(std::string const& name, std::string const& value) override;
        virtual std::string GetSetting(std::string const& name) override;
        virtual size_t GetSize() override;
        virtual size_t GetRecordCount(EventLatency latency) const override;
        virtual std::vector<StorageRecord> GetRecords(bool shutdown, EventLatency minLatency = EventLatency_Normal, unsigned maxCount = 0) override;
        virtual bool ResizeDb() override;

    protected:
        bool initializeDatabase();
        bool recreate(unsigned failureCode);

        std::vector<uint8_t> packageIdList(std::vector<std::string> const& ids);

        // Debug routine to print record count in the DB
        void printRecordCount();

        // Perform DB full check once a minute
        const size_t                DB_FULL_CHECK_TIME_MS = 5000;

    protected:
        mutable std::recursive_mutex m_lock;
        IOfflineStorageObserver*    m_observer;
        IRuntimeConfig&             m_config;
        ILogManager&                m_logManager;
        std::unique_ptr<SqliteDB>   m_db;

        bool                        isOpen();

        int                         m_pageSize;

        bool                        m_skipInitAndShutdown;
        bool                        m_isOpened;

        size_t                      m_stmtBeginTransaction;
        size_t                      m_stmtCommitTransaction;
        size_t                      m_stmtRollbackTransaction;
        size_t                      m_stmtGetPageCount;
        size_t                      m_stmtGetRecordCount;
        size_t                      m_stmtGetRecordCountBylatency;
        size_t                      m_stmtPerTenantTrimCount;
        size_t                      m_stmtTrimEvents_percent;
        size_t                      m_stmtDeleteEvents_ids;
        size_t                      m_stmtReleaseExpiredEvents;
        size_t                      m_stmtSelectEvents;
        size_t                      m_stmtSelectEventAtShutdown;
        size_t                      m_stmtSelectEventsMinlatency;
        size_t                      m_stmtReserveEvents;
        size_t                      m_stmtReleaseEvents_ids_retryCountDelta;
        size_t                      m_stmtDeleteEventsRetried_maxRetryCount;
        size_t                      m_stmtSelectEventsRetried_maxRetryCount;
        size_t                      m_stmtInsertEvent_id_tenant_prio_ts_data;
        size_t                      m_stmtInsertSetting_name_value;
        size_t                      m_stmtDeleteSetting_name;
        size_t                      m_stmtSelectSetting_name;
        unsigned                    m_lastReadCount;
        std::string                 m_offlineStorageFileName;
        unsigned                    m_DbSizeNotificationLimit;
        size_t                      m_DbSizeHeapLimit;
        size_t                      m_DbSizeLimit;
        size_t                      m_DbSizeEstimate;
        uint64_t                    m_isStorageFullNotificationSendTime;

    protected:
        MATSDK_LOG_DECL_COMPONENT_CLASS();
    };


} ARIASDK_NS_END
#endif