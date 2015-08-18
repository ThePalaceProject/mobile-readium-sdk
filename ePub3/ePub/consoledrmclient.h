/*************************************************************************
 *
 * ADOBE CONFIDENTIAL
 * ___________________
 *
 *  Copyright 2015 Adobe Systems Incorporated
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Adobe Systems Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Adobe Systems Incorporated and its
 * suppliers and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Adobe Systems Incorporated.
 **************************************************************************/

#if defined(FEATURE_DRM_CONNECTOR)

#ifndef _CONSOLEDRMCLIENT_H
#define _CONSOLEDRMCLIENT_H

#include "dp_all.h"
#include "connectorHelperFns.h"

class PasshashInputData : public dp::RefCounted
{
public: 
	PasshashInputData(const dp::String& user, const dp::String& password, int timeout) :
		m_refCount(0),
		m_user(user),
		m_password(password),
		m_timeout(timeout),
		m_next(dp::ref<PasshashInputData>())
	{
	}

	void addRef()
	{
		m_refCount++;
	}

	void release()
	{
		if ( --m_refCount == 0 ) 
			delete this;
	}

	int m_refCount;
	dp::String m_user;
	dp::String m_password;
	int m_timeout;
	dp::ref<PasshashInputData> m_next;
};

class ConsoleDRMProcessorClient : public dpdrm::DRMProcessorClient, public dptimer::TimerClient
{
public:

	ConsoleDRMProcessorClient(dpdev::Device * device)
	{

        dpdev::DeviceProvider *deviceProvider = dpdev::DeviceProvider::getProvider(0);
        dpdev::Device *deviceSelf = deviceProvider->getDevice(0);
        
        m_processor = dpdrm::DRMProvider::getProvider()->createDRMProcessor(this, device);
		m_passhashHead = dp::ref<PasshashInputData>();
		m_passhashTail = dp::ref<PasshashInputData>();
        m_fulfilledFilePath = dp::String();
        m_error = dp::String();
	}

	~ConsoleDRMProcessorClient()
	{
		if (m_processor)
		{
			m_processor->release();
			m_processor = NULL;
		}
	}

	virtual void workflowsDone( unsigned int workflows, const dp::Data& followUp )
	{
		//std::cout << "Workflow " << workflowToString(workflows).utf8() << " is done." << std::endl;
        //printf("Workflow %s is done.\n", workflowToString(workflows).utf8());
		if (followUp.length() > 0)
		{
			dp::String str(reinterpret_cast<const char *>(followUp.data()), followUp.length());
			//std::cout << "Data:" << std::endl << str.utf8() << std::endl;
            //printf("Data:\n%s\n", str.utf8());

		}
	}

	virtual void timerFired( dptimer::Timer * timer )
	{
		timer->release();
		deliverPasshash();
	}

	virtual void requestPasshash( const dp::ref<dpdrm::FulfillmentItem>& fulfillmentItem )
	{
		if (!m_passhashHead || !m_passhashHead->m_timeout)
		{
			deliverPasshash();
		}
		else
		{
			dptimer::TimerProvider * provider = dptimer::TimerProvider::getProvider();
			dptimer::Timer * timer = provider->createTimer(this);
			timer->setTimeout(m_passhashHead->m_timeout);
		}
	}

	virtual void requestInput( const dp::Data& inputXHTML )
	{
	}

	virtual void requestConfirmation( const dp::String& code )
	{
	}

	virtual void reportWorkflowProgress( unsigned int workflow, const dp::String& title, double progress )
	{
        //printf("Workflow %s PROGRESS: %s is done.\n", workflowToString(workflows).utf8(), progress);

	}

	virtual void reportWorkflowError( unsigned int workflow, const dp::String& errorCode )
	{
        // Since connector is part of the RMSDK package, we might run into these errors,
        // which should be ignored if we are using just the connector functionality and not complete RMSDK.
        // Hence, We are bypassing the 2 errors here.
        if (!((strncmp("E_ADEPT_DOCUMENT_CREATE_ERROR", errorCode.utf8(), strlen("E_ADEPT_DOCUMENT_CREATE_ERROR")) == 0) ||
            (strncmp("E_ADEPT_NOT_READY", errorCode.utf8(), strlen("E_ADEPT_NOT_READY")) == 0
            )))
        {
            m_error = errorCode;
        }
	}

	virtual void reportFollowUpURL( unsigned int workflow, const dp::String& url )
	{
	}

    virtual void reportDownloadCompleted( const dp::ref<dpdrm::FulfillmentItem>& fulfillmentItem, const dp::String& url )
    {
        m_error = dp::String();

        if(fulfillmentItem)
        {
            uft::String title = fulfillmentItem->getMetadata("DC.title");
            uft::String format = fulfillmentItem->getMetadata("DC.format");
            uft::String bookExt = "epub";
            if(format == "application/pdf")
                bookExt = "pdf";
            dp::ref<dpdrm::Rights> rights = fulfillmentItem->getRights();
            // uniqueFileName will prepend the Documents path in output path.
            dp::String dstPath = connectorutils::uniqueFileName(dp::String(title + "." + bookExt));

            if(rights)
            {
                dp::Data rightsData = rights->serialize();
                uft::String fileUrl = dstPath.uft() + "_rights.xml";
                connectorutils::writeFile(fileUrl.utf8(), rightsData, false);
            }
            
            //Copy the fulfilled book from temp folder to book2png folder
            dp::Data data = connectorutils::readFile(url.utf8(), false);
            connectorutils::writeFile(dstPath.utf8(), data, false);
            m_fulfilledFilePath = dstPath;
        }
        //printf("reportDownloadCompleted\n");
    }
    
    dp::String getFulfilledFilePath()
    {
        return m_fulfilledFilePath;
    }

    dp::String getErrorInWorkflow()
    {
        return m_error;
    }
    
    void resetError()
    {
        m_error = dp::String();
    }

	dpdrm::DRMProcessor *getDRMProcessor()
	{
		return m_processor;
	}

	void queuePasshashInput (const dp::String& user, const dp::String& password, int timeout)
	{
		dp::ref<PasshashInputData> temp = new PasshashInputData(user, password, timeout);
		
		if (m_passhashTail != NULL)
			m_passhashTail->m_next = temp;
		
		m_passhashTail = temp;

		if (m_passhashHead == NULL)
			m_passhashHead = temp;
	}

private:

	void deliverPasshash() 
	{
		dp::Data passhash = dp::Data();
		if (m_passhashHead != NULL)
		{
			dp::ref<PasshashInputData> temp = m_passhashHead;
			m_passhashHead = temp->m_next;
			if (m_passhashHead == NULL)
				m_passhashTail = m_passhashHead;

			if (temp->m_user.length() > 0)
				passhash = m_processor->calculatePasshash(temp->m_user, temp->m_password);
		}
		m_processor->providePasshash(passhash);
	}

	dp::String workflowToString(int workflow)
	{
		switch (workflow) 
		{
		case dpdrm::DW_SIGN_IN: return "DW_SIGN_IN";
		case dpdrm::DW_AUTH_SIGN_IN: return "DW_AUTH_SIGN_IN"; 
		case dpdrm::DW_ADD_SIGN_IN: return "DW_ADD_SIGN_IN";
		case dpdrm::DW_ACTIVATE: return "DW_ACTIVATE";
		case dpdrm::DW_FULFILL: return "DW_FULFILL";
		case dpdrm::DW_ENABLE_CONTENT: return "DW_ENABLE_CONTENT";
		case dpdrm::DW_LOAN_RETURN: return "DW_LOAN_RETURN";
		case dpdrm::DW_UPDATE_LOANS: return "DW_UPDATE_LOANS";
		case dpdrm::DW_DOWNLOAD: return "DW_DOWNLOAD";
		case dpdrm::DW_JOIN_ACCOUNTS: return "DW_JOIN_ACCOUNTS";
		case dpdrm::DW_GET_CREDENTIAL_LIST: return "DW_GET_CREDENTIAL_LIST";
		case dpdrm::DW_NOTIFY: return "DW_NOTIFY";
		}
		return "";
	}

	dpdrm::DRMProcessor * m_processor;
	dp::ref<PasshashInputData> m_passhashHead;
	dp::ref<PasshashInputData> m_passhashTail;
    dp::String m_fulfilledFilePath;
    dp::String m_error;
};

#endif // _CONSOLEDRMCLIENT_H
#endif //#if defined(FEATURE_DRM_CONNECTOR)

