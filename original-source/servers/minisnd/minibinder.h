
#if !defined(minibinder_h)
#define minibinder_h

#include <Binder.h>
#include <vector>

class minibinder;

extern void make_binder();
extern void break_binder();
extern void update_binder_property_vol(int source, float value, bool mute);
extern void update_binder_property_adc(int value);

class minibinder : public BinderNode
{
public:
		minibinder();

		void						update_property(const char * name, const property & value);

		virtual	status_t			OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t			NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t			CloseProperties(void *cookie);

		virtual	put_status_t		WriteProperty(const char *name, const property &prop);
		virtual	get_status_t		ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

private:

		~minibinder();

		int32						m_beep;
		struct info
		{
			char					name[16];
			BinderNode::property	value;
		};
		std::vector<info> m_properties;		//	could turn this into a map<BString, property>
										//	but then we'd lose case insensitivity
};


#endif	//	minibinder_h
