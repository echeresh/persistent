namespace persistent
{
    template <class key_type, class value_type>
    struct key_value_entry
    {
        key_type key;
        value_type value;

        key_value_entry(const key_type& key, const value_type& value) :
            key(key),
            value(value)
        {
        }
    };
}
