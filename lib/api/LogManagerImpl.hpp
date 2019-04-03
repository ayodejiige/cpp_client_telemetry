// Copyright (c) Microsoft. All rights reserved.
#ifndef LOGMANAGERIMPL_HPP
#define LOGMANAGERIMPL_HPP

#include "LogConfiguration.hpp"
#include "config/RuntimeConfig_Default.hpp"

#include "system/Contexts.hpp"

#include "IHttpClient.hpp"
#include "ILogManager.hpp"

#include "api/Logger.hpp"
#include "api/ContextFieldsProvider.hpp"

#include "filter/EventFilterRegulator.hpp"

#include "DebugEvents.hpp"
#include <memory>

#include "IBandwidthController.hpp"
#include "api/AuthTokensController.hpp"

#include "LogSessionData.hpp"

#include <mutex>
#include <set>

namespace ARIASDK_NS_BEGIN
{

    class ITelemetrySystem;

    class DiagLevelFilter final {
    public:
        DiagLevelFilter():
            m_level(DIAG_LEVEL_DEFAULT),
            m_levelMin(DIAG_LEVEL_DEFAULT_MIN),
            m_levelMax(DIAG_LEVEL_DEFAULT_MAX),
            m_levelSet({})
        {
        }

        /// <summary>
        /// Internal method that allows to obtain the default level
        /// </summary>
        uint8_t GetDefaultLevel() const
        {
            return m_level;
        }

        /// <summary>
        /// Verify if logging is enabled for given level
        /// </summary>
        /// <param name="level">Diagnostic level.</param>
        bool IsLevelEnabled(uint8_t level) const
        {
            if (!m_levelSet.empty())
            {
                return m_levelSet.find(level) != m_levelSet.end();
            }
            return m_levelMin <= m_levelMax && m_levelMin <= level && level <= m_levelMax;
        }

        /// <summary>
        /// Method that checks if the filtering has been enabled
        /// </summary>
        bool IsLevelFilterEnabled() const
        {
            return !m_levelSet.empty() || m_levelMin != DIAG_LEVEL_DEFAULT_MIN || m_levelMax != DIAG_LEVEL_DEFAULT_MAX || m_level != DIAG_LEVEL_DEFAULT;
        }

        /// <summary>
        /// Method that allows to set the filter for the LogManager
        /// <param name="defaultLevel">Diag level for the LogManager</param>
        /// <param name="levelMin">Min level to enable</param>
        /// <param name="levelMax">Max level to enable</param>
        /// </summary>
        void SetFilter(uint8_t defaultLevel, uint8_t levelMin, uint8_t levelMax)
        {
            m_level    = defaultLevel;
            m_levelMin = levelMin;
            m_levelMax = levelMax;
        }

        /// <summary>
        /// Method that allows to set the filter for the LogManager
        /// <param name="defaultLevel">Diag level for the LogManager</param>
        /// <param name="allowedLevels">Set with the enabled levels</param>
        /// </summary>
        void SetFilter(uint8_t defaultLevel, const std::set<uint8_t>& allowedLevels)
        {
            m_level    = defaultLevel;
            m_levelSet = allowedLevels;
        }

    private:
        uint8_t                                m_levelMin;
        uint8_t                                m_levelMax;
        uint8_t                                m_level;
        std::set<uint8_t>                      m_levelSet;
    };

    class ILogManagerInternal : public ILogManager {
    public:
        static std::recursive_mutex     managers_lock;
        static std::set<ILogManager*>   managers;

        virtual void sendEvent(IncomingEventContextPtr const& event) = 0;
        virtual const ContextFieldsProvider& GetContext() = 0;
        virtual const DiagLevelFilter& GetLevelFilter() = 0;
    };

    class Logger;

    class LogManagerImpl : public ILogManagerInternal {

    public:

        LogManagerImpl(ILogConfiguration& configuration);

        virtual ~LogManagerImpl() override;

        /**
         * ILogController - state management methods
         */
        virtual void Configure() override;

        virtual void FlushAndTeardown() override;

        virtual status_t Flush() override;
        virtual status_t UploadNow() override;
        virtual status_t PauseTransmission() override;
        virtual status_t ResumeTransmission() override;
        virtual status_t SetTransmitProfile(TransmitProfile profile) override;
        virtual status_t SetTransmitProfile(const std::string& profile) override;
        virtual status_t LoadTransmitProfiles(const std::string& profiles_json) override;
        virtual status_t ResetTransmitProfiles() override;
        virtual const std::string& GetTransmitProfileName() override;

        /**
         * Semantic Context methods
         */
        virtual ISemanticContext& GetSemanticContext() override;

        virtual status_t SetContext(std::string const& name, std::string const& value, PiiKind piiKind = PiiKind_None) override;

        virtual status_t SetContext(const std::string& name, double value, PiiKind piiKind = PiiKind_None) override;

        virtual status_t SetContext(const std::string& name, int64_t value, PiiKind piiKind = PiiKind_None) override;

        virtual inline status_t SetContext(const std::string& name, const char *value, PiiKind piiKind = PiiKind_None) override { const std::string val(value); return SetContext(name, val, piiKind); };

        virtual inline status_t SetContext(const std::string& name, int8_t  value, PiiKind piiKind = PiiKind_None) override { return SetContext(name, (int64_t)value, piiKind); }

        virtual inline status_t SetContext(const std::string& name, int16_t value, PiiKind piiKind = PiiKind_None) override { return SetContext(name, (int64_t)value, piiKind); }

        virtual inline status_t SetContext(const std::string& name, int32_t value, PiiKind piiKind = PiiKind_None) override { return SetContext(name, (int64_t)value, piiKind); }

        virtual inline status_t SetContext(const std::string& name, uint8_t  value, PiiKind piiKind = PiiKind_None) override { return SetContext(name, (int64_t)value, piiKind); }

        virtual inline status_t SetContext(const std::string& name, uint16_t value, PiiKind piiKind = PiiKind_None) override { return SetContext(name, (int64_t)value, piiKind); }

        virtual inline status_t SetContext(const std::string& name, uint32_t value, PiiKind piiKind = PiiKind_None) override { return SetContext(name, (int64_t)value, piiKind); }

        virtual inline status_t SetContext(const std::string& name, uint64_t value, PiiKind piiKind = PiiKind_None) override { return SetContext(name, (int64_t)value, piiKind); }

        virtual status_t SetContext(const std::string& name, bool value, PiiKind piiKind = PiiKind_None) override;

        virtual status_t SetContext(const std::string& name, time_ticks_t value, PiiKind piiKind = PiiKind_None) override;

        virtual status_t SetContext(const std::string& name, GUID_t value, PiiKind piiKind = PiiKind_None) override;

        virtual ILogConfiguration & GetLogConfiguration() override;

        virtual ILogger* GetLogger(std::string const& tenantToken, std::string const& source = std::string(), std::string const& scopeId = std::string()) override;

        LogSessionData* GetLogSessionData() override;

        ILogController *GetLogController(void) override;

        IAuthTokensController* GetAuthTokensController() override;

        /// <summary>
        /// Adds the event listener.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="listener">The listener.</param>
        virtual void AddEventListener(DebugEventType type, DebugEventListener &listener) override;

        /// <summary>
        /// Removes the event listener.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="listener">The listener.</param>
        virtual void RemoveEventListener(DebugEventType type, DebugEventListener &listener) override;

        /// <summary>
        /// Dispatches the event.
        /// </summary>
        /// <param name="evt">The evt.</param>
        /// <returns></returns>
        virtual bool DispatchEvent(DebugEvent evt) override;

        ///
        virtual bool AttachEventSource(DebugEventSource & other) override;

        ///
        virtual bool DetachEventSource(DebugEventSource & other) override;
        
        /// <summary>
        /// Sets the exclusion filter.
        /// </summary>
        /// <param name="tenantToken">The tenant token.</param>
        /// <param name="filterStrings">The filter strings.</param>
        /// <param name="filterCount">The filter count.</param>
        /// <returns></returns>
        status_t SetExclusionFilter(const char* tenantToken, const char** filterStrings, uint32_t filterCount) override;
        

        /// <summary>
        /// Sets the exclusion filter.
        /// </summary>
        /// <param name="tenantToken">The tenant token.</param>
        /// <param name="filterStrings">The filter strings.</param>
        /// <param name="filterRates">The filter rates.</param>
        /// <param name="filterCount">The filter count.</param>
        /// <returns></returns>
        status_t SetExclusionFilter(const char* tenantToken, const char** filterStrings, const uint32_t* filterRates, uint32_t filterCount) override;

        /// <summary>
        /// Adds the incoming event.
        /// </summary>
        /// <param name="event">The event.</param>
        virtual void sendEvent(IncomingEventContextPtr const& event) override;

        void SetLevelFilter(uint8_t defaultLevel, uint8_t levelMin, uint8_t levelMax) override;

        void SetLevelFilter(uint8_t defaultLevel, const std::set<uint8_t>& allowedLevels) override;

        /// <summary>
        /// Get a reference to this log manager diagnostic level filter
        /// </summary>
        virtual const DiagLevelFilter& GetLevelFilter() override;

        /// <summary>
        /// Get a reference to this log manager instance ContextFieldsProvider
        /// </summary>
        /// <param name="level">Diagnostic level.</param>
        virtual const ContextFieldsProvider& GetContext() override
        {
            return m_context;
        }

protected:

        MATSDK_LOG_DECL_COMPONENT_CLASS();

        std::mutex                             m_lock;
        std::map<std::string, Logger*>         m_loggers;
        ContextFieldsProvider                  m_context;

        IHttpClient*                           m_httpClient;
        std::unique_ptr<IHttpClient>           m_ownHttpClient;

        IRuntimeConfig*                        m_config;
        ILogConfiguration&                     m_logConfiguration;

        IBandwidthController*                  m_bandwidthController;
        std::unique_ptr<IBandwidthController>  m_ownBandwidthController;

        AuthTokensController                   m_authTokensController;

        std::unique_ptr<IOfflineStorage>       m_offlineStorage;
        std::unique_ptr<LogSessionData>        m_logSessionData;
        std::unique_ptr<ITelemetrySystem>      m_system;

        EventFilterRegulator                   m_eventFilterRegulator;

        bool                                   m_alive;

        DebugEventSource                       m_debugEventSource;
        DiagLevelFilter                        m_diagLevelFilter;
    };


} ARIASDK_NS_END

#endif