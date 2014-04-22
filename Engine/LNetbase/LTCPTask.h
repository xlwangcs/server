/**
 * \file
 * \version  $Id: LTCPTask.h  $
 * \author  
 * \date 
 * \brief ��װʵ���̳߳أ����ڴ��������ӷ�����
 *
 * 
 */


#ifndef _zTCPTask_h_
#define _zTCPTask_h_

#include <string>
#include <vector>
#include <queue>
#include <list>
#include <unistd.h>
#include <sys/timeb.h>

#include "LSocket.h"
#include "LCantCopy.h"
#include "LTime.h"

class LTCPTaskPool;

/**
 * \brief ����һ�������࣬���̳߳صĹ�����Ԫ
 *
 */
class LTCPTask : public LProcessor, private LCantCopy
{

	public:

		/**
		 * \brief ���ӶϿ���ʽ
		 *
		 */
		enum TerminateMethod
		{
			terminate_no,								/**< û�н������� */
			terminate_active,							/**< �ͻ��������Ͽ����ӣ���Ҫ�����ڷ������˼�⵽�׽ӿڹرջ����׽ӿ��쳣 */
			terminate_passive,							/**< �������������Ͽ����� */
		};

		/**
		 * \brief ���캯�������ڴ���һ������
		 *
		 *
		 * \param pool �������ӳ�ָ��
		 * \param sock �׽ӿ�
		 * \param addr ��ַ
		 * \param compress �ײ����ݴ����Ƿ�֧��ѹ��
		 * \param checkSignal �Ƿ���������·�����ź�
		 */
		LTCPTask(
				LTCPTaskPool *pool,
				const int sock,
				const struct sockaddr_in *addr = NULL,
				const bool compress = false,
				const bool checkSignal = true) : mSocket(sock, addr, compress), pool(pool), lifeTime(), _checkSignal(checkSignal), _ten_min(600), tick(false)
		{
			terminate = terminate_no;
			terminate_wait = false; 
			fdsradd = false; 
			buffered = false;
			state = notuse;
		}

		/**
		 * \brief ������������������һ������
		 *
		 */
		virtual ~LTCPTask() {}

#ifdef _USE_EPOLL_
		/**
		 * \brief ���Ӽ���¼���epoll������
		 * \param kdpfd epoll������
		 * \param events �����ӵ��¼�
		 * \param ptr �������
		 */
		void addEpoll(int kdpfd, __uint32_t events, void *ptr)
		{
			mSocket.addEpoll(kdpfd, events, ptr);
		}
		/**
		 * \brief ��epoll��������ɾ������¼�
		 * \param kdpfd epoll������
		 * \param events �����ӵ��¼�
		 */
		void delEpoll(int kdpfd, __uint32_t events)
		{
			mSocket.delEpoll(kdpfd, events);
		}
#else
		/**
		 * \brief ���pollfd�ṹ
		 * \param pfd �����Ľṹ
		 * \param events �ȴ����¼�����
		 */
		void fillPollFD(struct pollfd &pfd, short events)
		{
			//mSocket.fillPollFD(pfd, events);
		}
#endif

		/**
		 * \brief ����Ƿ���֤��ʱ
		 *
		 *
		 * \param ct ��ǰϵͳʱ��
		 * \param interval ��ʱʱ�䣬����
		 * \return ����Ƿ�ɹ�
		 */
		bool checkVerifyTimeout(const LRTime &ct, const unsigned long long interval = 2000) const
		{
			return (lifeTime.elapse(ct) > interval);
		}

		/**
		 * \brief ����Ƿ��Ѿ�������¼�
		 *
		 * \return �Ƿ����
		 */
		bool isFdsrAdd()
		{
			return fdsradd;
		}
		/**
		 * \brief ���ü�����¼���־
		 *
		 * \return �Ƿ����
		 */
		bool fdsrAdd()
		{
			 fdsradd=true;
			 return fdsradd;
		}


		/**
		 * \brief ������֤����
		 *
		 * ������Ҫ�����������������֤һ��TCP���ӣ�ÿ��TCP���ӱ���ͨ����֤���ܽ�����һ�������׶Σ�ȱʡʹ��һ���յ�ָ����Ϊ��ָ֤��
		 * <pre>
		 * int retcode = mSocket.recvToBuf_NoPoll();
		 * if (retcode > 0)
		 * {
		 * 		unsigned char pstrCmd[LSocket::MAX_DATASIZE];
		 * 		int nCmdLen = mSocket.recvToCmd_NoPoll(pstrCmd, sizeof(pstrCmd));
		 * 		if (nCmdLen <= 0)
		 * 			//����ֻ�Ǵӻ���ȡ���ݰ������Բ��������û������ֱ�ӷ���
		 * 			return 0;
		 * 		else
		 * 		{
		 * 			LSocket::t_NullCmd *ptNullCmd = (LSocket::t_NullCmd *)pstrCmd;
		 * 			if (LSocket::null_opcode == ptNullCmd->opcode)
		 * 			{
		 * 				std::cout << "�ͻ�������ͨ����֤" << std::endl;
		 * 				return 1;
		 * 			}
		 * 			else
		 * 			{
		 * 				return -1;
		 * 			}
		 * 		}
		 * }
		 * else
		 * 		return retcode;
		 * </pre>
		 *
		 * \return ��֤�Ƿ�ɹ���1��ʾ�ɹ������Խ�����һ��������0����ʾ��Ҫ�����ȴ���֤��-1��ʾ�ȴ���֤ʧ�ܣ���Ҫ�Ͽ�����
		 */
		virtual int verifyConn()
		{
			return 1;
		}

		/**
		 * \brief �ȴ������߳�ͬ����֤������ӣ���Щ�̳߳ز���Ҫ�ⲽ�����Բ����������������ȱʡʼ�շ��سɹ�
		 *
		 * \return �ȴ��Ƿ�ɹ���1��ʾ�ɹ������Խ�����һ��������0����ʾ��Ҫ�����ȴ���-1��ʾ�ȴ�ʧ�ܻ��ߵȴ���ʱ����Ҫ�Ͽ�����
		 */
		virtual int waitSync()
		{
			return 1;
		}

		/**
		 * \brief �����Ƿ�ɹ������ճɹ��Ժ���Ҫɾ�����TCP���������Դ
		 *
		 * \return �����Ƿ�ɹ���1��ʾ���ճɹ���0��ʾ���ղ��ɹ�
		 */
		virtual int recycleConn()
		{
			return 1;
		}

		/**
		 * \brief һ������������֤�Ȳ�������Ժ���Ҫ���ӵ�ȫ��������
		 *
		 * ���ȫ���������ⲿ����
		 *
		 */
		virtual void addToContainer() {}

		/**
		 * \brief ���������˳���ʱ����Ҫ��ȫ��������ɾ��
		 *
		 * ���ȫ���������ⲿ����
		 *
		 */
		virtual void removeFromContainer() {}

		/**
		 * \brief ���ӵ��ⲿ���������������Ҫ��֤������ӵ�Ψһ��
		 *
		 * \return �����Ƿ�ɹ�
		 */
		virtual bool uniqueAdd()
		{
			return true;
		}

		/**
		 * \brief ���ⲿ����ɾ�������������Ҫ��֤������ӵ�Ψһ��
		 *
		 * \return ɾ���Ƿ�ɹ�
		 */
		virtual bool uniqueRemove()
		{
			return true;
		}

		/**
		 * \brief ����Ψһ����֤ͨ�����
		 *
		 */
		void setUnique()
		{
			uniqueVerified = true;
		}

		/**
		 * \brief �ж��Ƿ��Ѿ�ͨ����Ψһ����֤
		 *
		 * \return �Ƿ��Ѿ�ͨ����Ψһ�Ա��
		 */
		bool isUnique() const
		{
			return uniqueVerified;
		}

		/**
		 * \brief �ж��Ƿ������߳�����Ϊ�ȴ��Ͽ�����״̬
		 *
		 * \return true or false
		 */
		bool isTerminateWait()
		{
			return terminate_wait; 
		}
		

		/**
		 * \brief �ж��Ƿ������߳�����Ϊ�ȴ��Ͽ�����״̬
		 *
		 * \return true or false
		 */
		void TerminateWait()
		{
			terminate_wait=true; 
		}

		/**
		 * \brief �ж��Ƿ���Ҫ�ر�����
		 *
		 * \return true or false
		 */
		bool isTerminate() const
		{
			return terminate_no != terminate;
		}

		/**
		 * \brief ��Ҫ�����Ͽ��ͻ��˵�����
		 *
		 * \param method ���ӶϿ���ʽ
		 */
		virtual void Terminate(const TerminateMethod method = terminate_passive)
		{
			terminate = method;
		}

		virtual bool sendCmd(const void *, int);
		bool sendCmdPack(const void *, int);
		virtual bool ListeningRecv(bool);
		virtual bool ListeningSend();

		/**
		 * \brief ��������״̬
		 *
		 */
		enum zTCPTask_State
		{
			notuse		=	0,							/**< ���ӹر�״̬ */
			verify		=	1,							/**< ������֤״̬ */
			sync		=	2,							/**< �ȴ�������������������֤��Ϣͬ�� */
			okay		=	3,							/**< ���Ӵ����׶Σ���֤ͨ���ˣ�������ѭ�� */
			recycle		=	4							/**< �����˳�״̬������ */
		};

		/**
		 * \brief ��ȡ��������ǰ״̬
		 * \return ״̬
		 */
		const zTCPTask_State getState() const
		{
			return state;
		}

		/**
		 * \brief ������������״̬
		 * \param state ��Ҫ���õ�״̬
		 */
		void setState(const zTCPTask_State state)
		{
			this->state = state;
		}

		void getNextState();
		void resetState();

		/**
		 * \brief ���״̬���ַ�������
		 *
		 *
		 * \param state ״̬
		 * \return ����״̬���ַ�������
		 */
		const char *getStateString(const zTCPTask_State state) const
		{
			const char *retval = NULL;

			switch(state)
			{
				case notuse:
					retval = "notuse";
					break;
				case verify:
					retval = "verify";
					break;
				case sync:
					retval = "sync";
					break;
				case okay:
					retval = "okay";
					break;
				case recycle:
					retval = "recycle";
					break;
				default:
					retval = "none";
					break;
			}

			return retval;
		}

		/**
		 * \brief �������ӵ�IP��ַ
		 * \return ���ӵ�IP��ַ
		 */
		const char *getIP() const
		{
			return mSocket.getIP();
		}
		const DWORD getAddr() const
		{
			return mSocket.getAddr();
		}

		const unsigned short getPort()
		{
			return mSocket.getPort();
		}

		/**
		 * \brief �Ƿ�������������·�����ź�
		 * \return true or false
		 */
		const bool ifCheckSignal() const
		{
			return _checkSignal;
		}

		/**
		 * \brief �������źŷ��ͼ��
		 *
		 * \return ����Ƿ�ɹ�
		 */
		bool checkInterval(const LRTime &ct)
		{
			return _ten_min(ct);
		}
		void clearCheckSignal()
		{   
			_checkSignal = false;
		} 

		/**
		 * \brief �������źţ���������ź��ڹ涨ʱ���ڷ��أ���ô���·��Ͳ����źţ�û�з��صĻ�����TCP�����Ѿ�������
		 *
		 * \return true����ʾ���ɹ���false����ʾ���ʧ�� 
		 */
		bool checkTick() const
		{
			return tick;
		}

		/**
		 * \brief �����ź��Ѿ�������
		 *
		 */
		void clearTick()
		{
			tick = false;
		}

		/**
		 * \brief ���Ͳ����źųɹ�
		 *
		 */
		void setTick()
		{
			tick = true;
		}
		LTCPTaskPool *getPool()
		{
			return pool; 
		}
		void setSocketEnc(CEncrypt::encMethod m,const unsigned char* key,uint8_t len,uint8_t round)
		{
			mSocket.setEncMethod(m);
			mSocket.set_key_rc5(key, 16, 12);
		}

		void checkSignal(const LRTime &ct);

		static CmdAnalysis analysis;
	protected:

		bool buffered;									/**< ����ָ���Ƿ񻺳� */
		LSocket mSocket;								/**< �ײ��׽ӿ� */
		zTCPTask_State state;							/**< ����״̬ */

	private:

		LTCPTaskPool *pool;								/**< ���������ĳ� */
		TerminateMethod terminate;						/**< �Ƿ�������� */
		bool terminate_wait;							/**< �����߳����õȴ��Ͽ�����״̬,��pool�߳����öϿ�����״̬ */
		bool fdsradd;									/**< ���¼����ӱ�־ */
		LRTime lifeTime;								/**< ���Ӵ���ʱ���¼ */

		bool uniqueVerified;							/**< �Ƿ�ͨ����Ψһ����֤ */
		bool _checkSignal;						/**< �Ƿ�����·����ź� */
		Timer _ten_min;
		bool tick;

};

#endif
