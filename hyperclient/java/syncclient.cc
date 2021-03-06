// Copyright (c) 2012, Cornell University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of HyperDex nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// STL
#include <map>
#include <string>

// e
#include <e/guard.h>

// HyperClient
#include "hyperclient/hyperclient.h"
#include "hyperclient/java/syncclient.h"

HyperClient :: HyperClient(const char* coordinator, in_port_t port)
    : m_client(coordinator, port)
{
}

HyperClient :: ~HyperClient() throw ()
{
}

hyperclient_returncode
HyperClient :: get(const std::string& space,
                   const std::string& key,
                   std::map<std::string, std::string>* svalues,
                   std::map<std::string, uint64_t>* nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    hyperclient_attribute* attrs = NULL;
    size_t attrs_sz = 0;

    id = m_client.get(space.c_str(),
                      key.data(),
                      key.size(),
                      &stat1,
                      &attrs,
                      &attrs_sz);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(id == lid);
    e::guard g = e::makeguard(free, attrs); g.use_variable();

    for (size_t i = 0; i < attrs_sz; ++i)
    {
        if ( attrs[i].datatype == HYPERDATATYPE_STRING )
        {
            svalues->insert(std::make_pair(std::string(attrs[i].attr),
                                           std::string(attrs[i].value,
                                                       attrs[i].value_sz)));
        }
        else if ( attrs[i].datatype == HYPERDATATYPE_INT64 )
        {
            nvalues->insert(std::make_pair(std::string(attrs[i].attr),
                            le64toh(*((uint64_t *)(attrs[i].value)))));
        }
    }

    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: put(const std::string& space,
                   const std::string& key,
                   const std::map<std::string, std::string>& svalues,
                   const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, std::string>::const_iterator ci = svalues.begin();
            ci != svalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.put(space.c_str(),
                      key.data(),
                      key.size(),
                      &attrs.front(),
                      attrs.size(),
                      &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: condput(const std::string& space, 
                           const std::string& key,  
                           const std::map<std::string, std::string>& condattrs_sval,
                           const std::map<std::string, uint64_t>& condattrs_nval,
                           const std::map<std::string, std::string>& attrs_sval,
                           const std::map<std::string, uint64_t>& attrs_nval)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> condattrs;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> cond_nums;
    cond_nums.reserve(condattrs_nval.size());
    std::vector<uint64_t> attr_nums;
    attr_nums.reserve(attrs_nval.size());

    for (std::map<std::string, std::string>::const_iterator ci = condattrs_sval.begin();
            ci != condattrs_sval.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        condattrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = condattrs_nval.begin();
            ci != condattrs_nval.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        cond_nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&cond_nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        condattrs.push_back(at);
    }

    for (std::map<std::string, std::string>::const_iterator ci = attrs_sval.begin();
            ci != attrs_sval.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = attrs_nval.begin();
            ci != attrs_nval.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        attr_nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&attr_nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.condput(space.c_str(),
                          key.data(),
                          key.size(),
                          &condattrs.front(),
                          condattrs.size(),
                          &attrs.front(),
                          attrs.size(),
                          &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;


}

hyperclient_returncode
HyperClient :: atomic_add(const std::string& space,
                          const std::string& key, 
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_add(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: atomic_sub(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_sub(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: atomic_mul(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_mul(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: atomic_div(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_div(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);
if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: atomic_mod(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_mod(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: atomic_and(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {   
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_and(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: atomic_or(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_or(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: atomic_xor(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.atomic_xor(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: string_prepend(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, std::string>& svalue)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;

    for (std::map<std::string, std::string>::const_iterator ci = svalue.begin();
            ci != svalue.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    id = m_client.string_prepend(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}
hyperclient_returncode
HyperClient :: string_append(const std::string& space,
                             const std::string& key,
                             const std::map<std::string, std::string>& svalue)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;

    for (std::map<std::string, std::string>::const_iterator ci = svalue.begin();
            ci != svalue.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    id = m_client.string_append(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: list_lpush(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, std::string>& svalues,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, std::string>::const_iterator ci = svalues.begin();
            ci != svalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.list_lpush(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: list_rpush(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, std::string>& svalues,
                          const std::map<std::string, uint64_t>& nvalues)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, std::string>::const_iterator ci = svalues.begin();
            ci != svalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }
    id = m_client.list_rpush(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: set_add(const std::string& space,
                       const std::string& key,
                       const std::map<std::string, std::string>& svalues,
                       const std::map<std::string, uint64_t>& nvalues)

{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, std::string>::const_iterator ci = svalues.begin();
            ci != svalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.set_add(space.c_str(),
                          key.data(),
                          key.size(),
                          &attrs.front(),
                          attrs.size(),
                          &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: set_remove(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, std::string>& svalues,
                          const std::map<std::string, uint64_t>& nvalues)

{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, std::string>::const_iterator ci = svalues.begin();
            ci != svalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }
    id = m_client.set_remove(space.c_str(),
                             key.data(),
                             key.size(),
                             &attrs.front(),
                             attrs.size(),
                             &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
} 

hyperclient_returncode
HyperClient :: set_intersect(const std::string& space,
                             const std::string& key,
                             const std::map<std::string, std::string>& svalues,
                             const std::map<std::string, uint64_t>& nvalues)

{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, std::string>::const_iterator ci = svalues.begin();
            ci != svalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.set_intersect(space.c_str(),
                                key.data(),
                                key.size(),
                                &attrs.front(),
                                attrs.size(),
                                &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: set_union(const std::string& space,
                             const std::string& key,
                             const std::map<std::string, std::string>& svalues,
                             const std::map<std::string, uint64_t>& nvalues)

{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(nvalues.size());

    for (std::map<std::string, std::string>::const_iterator ci = svalues.begin();
            ci != svalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.data();
        at.value_sz = ci->second.size();
        at.datatype = HYPERDATATYPE_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, uint64_t>::const_iterator ci = nvalues.begin();
            ci != nvalues.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        nums.push_back(htole64(ci->second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_INT64;
        attrs.push_back(at);
    }

    id = m_client.set_union(space.c_str(),
                            key.data(),
                            key.size(),
                            &attrs.front(),
                            attrs.size(),
                            &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_add(const std::string& space,
                       const std::string& key,
                       const std::map<std::string, smpair>& spair,
                       const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, smpair>::const_iterator ci = spair.begin();
            ci != spair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        at.value = ci->second.second.c_str();
        at.value_sz = ci->second.second.size();
        at.datatype = HYPERDATATYPE_MAP_STRING_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_add(space.c_str(),
                            key.data(),
                            key.size(),
                            &attrs.front(),
                            attrs.size(),
                            &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_remove(const std::string& space,
                          const std::string& key,
                          const std::map<std::string, smpair>& spair,
                          const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, smpair>::const_iterator ci = spair.begin();
            ci != spair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        at.value = ci->second.second.c_str();
        at.value_sz = ci->second.second.size();
        at.datatype = HYPERDATATYPE_MAP_STRING_STRING;
        attrs.push_back(at);
    }

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }
    id = m_client.map_remove(space.c_str(),
                            key.data(),
                            key.size(),
                            &attrs.front(),
                            attrs.size(),
                            &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_atomic_add(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_atomic_add(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_atomic_sub(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_atomic_sub(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_atomic_mul(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }
    
    id = m_client.map_atomic_mul(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_atomic_div(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_atomic_div(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}


hyperclient_returncode
HyperClient :: map_atomic_mod(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_atomic_mod(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_atomic_and(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_atomic_and(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_atomic_or(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_atomic_or(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_atomic_xor(const std::string& space,
                              const std::string& key,
                              const std::map<std::string, nmpair>& npair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;
    std::vector<uint64_t> nums;
    nums.reserve(npair.size());

    for (std::map<std::string, nmpair>::const_iterator ci = npair.begin();
            ci != npair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        nums.push_back(htole64(ci->second.second));
        at.value = reinterpret_cast<const char*>(&nums.back());
        at.value_sz = sizeof(uint64_t);
        at.datatype = HYPERDATATYPE_MAP_STRING_INT64;
        attrs.push_back(at);
    }

    id = m_client.map_atomic_xor(space.c_str(),
                                 key.data(),
                                 key.size(),
                                 &attrs.front(),
                                 attrs.size(),
                                 &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_string_prepend(const std::string& space,
                                  const std::string& key,
                                  const std::map<std::string, smpair>& spair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;

    for (std::map<std::string, smpair>::const_iterator ci = spair.begin();
            ci != spair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        at.value = ci->second.second.c_str();
        at.value_sz = ci->second.second.size();
        at.datatype = HYPERDATATYPE_MAP_STRING_STRING;
        attrs.push_back(at);
    }

    id = m_client.map_string_prepend(space.c_str(),
                                     key.data(),
                                     key.size(),
                                     &attrs.front(),
                                     attrs.size(),
                                     &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: map_string_append(const std::string& space,
                                  const std::string& key,
                                  const std::map<std::string, smpair>& spair)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;
    std::vector<hyperclient_map_attribute> attrs;

    for (std::map<std::string, smpair>::const_iterator ci = spair.begin();
            ci != spair.end(); ++ci)
    {
        hyperclient_map_attribute at;
        at.attr = ci->first.c_str();
        at.map_key = ci->second.first.c_str();
        at.map_key_sz = ci->second.first.size();
        at.value = ci->second.second.c_str();
        at.value_sz = ci->second.second.size();
        at.datatype = HYPERDATATYPE_MAP_STRING_STRING;
        attrs.push_back(at);
    }

    id = m_client.map_string_append(space.c_str(),
                                     key.data(),
                                     key.size(),
                                     &attrs.front(),
                                     attrs.size(),
                                     &stat1);
    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}


hyperclient_returncode
HyperClient :: del(const std::string& space,
                   const std::string& key)
{
    int64_t id;
    hyperclient_returncode stat1 = HYPERCLIENT_A;
    hyperclient_returncode stat2 = HYPERCLIENT_B;

    id = m_client.del(space.c_str(), key.data(), key.size(), &stat1);

    if (id < 0)
    {
        assert(static_cast<unsigned>(stat1) >= 8448);
        assert(static_cast<unsigned>(stat1) < 8576);
        return stat1;
    }

    int64_t lid = m_client.loop(-1, &stat2);

    if (lid < 0)
    {
        assert(static_cast<unsigned>(stat2) >= 8448);
        assert(static_cast<unsigned>(stat2) < 8576);
        return stat2;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(stat1) >= 8448);
    assert(static_cast<unsigned>(stat1) < 8576);
    return stat1;
}

hyperclient_returncode
HyperClient :: range_search(const std::string& space,
                            const std::string& attr,
                            uint64_t lower,
                            uint64_t upper,
                            std::vector<std::map<std::string, std::string> >* sresults,
                            std::vector<std::map<std::string, uint64_t> >* nresults)
{
    hyperclient_range_query rn;
    rn.attr = attr.c_str();
    rn.lower = lower;
    rn.upper = upper;

    int64_t id;
    hyperclient_returncode status = HYPERCLIENT_A;
    hyperclient_attribute* attrs = NULL;
    size_t attrs_sz = 0;

    id = m_client.search(space.c_str(), NULL, 0, &rn, 1, &status, &attrs, &attrs_sz);

    if (id < 0)
    {
        return status;
    }

    int64_t lid;
    hyperclient_returncode lstatus = HYPERCLIENT_B;

    size_t pre_insert_size = 0; // Keep track of the number of "n-tuples" returned

    while ((lid = m_client.loop(-1, &lstatus)) == id)
    {
        if (status == HYPERCLIENT_SEARCHDONE)
        {
            return HYPERCLIENT_SUCCESS;
        }

        if (status != HYPERCLIENT_SUCCESS)
        {
            break;
        }

        for (size_t i = 0; i < attrs_sz; ++i)
        {
            if ( attrs[i].datatype == HYPERDATATYPE_STRING )
            {
                if (sresults->empty() || sresults->size() == pre_insert_size)
                {
                    sresults->push_back(std::map<std::string, std::string>());
                }

                sresults->back().insert(std::make_pair(attrs[i].attr, std::string(attrs[i].value, attrs[i].value_sz)));
            }
            else if ( attrs[i].datatype == HYPERDATATYPE_INT64 )
            {
                if (nresults->empty() || nresults->size() == pre_insert_size)
                {
                    nresults->push_back(std::map<std::string, uint64_t>());
                }

                nresults->back().insert(std::make_pair(attrs[i].attr, le64toh(*((uint64_t *)(attrs[i].value)))));
            }
        }

        pre_insert_size++;

        if (attrs)
        {
            hyperclient_destroy_attrs(attrs, attrs_sz);
        }

        attrs = NULL;
        attrs_sz = 0;
    }

    if (lid < 0)
    {
        assert(static_cast<unsigned>(lstatus) >= 8448);
        assert(static_cast<unsigned>(lstatus) < 8576);
        return lstatus;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(status) >= 8448);
    assert(static_cast<unsigned>(status) < 8576);
    return status;
}

hyperclient_returncode
HyperClient :: search(const std::string& space,
                      const std::map<std::string, val_type>& eq_attr, //This map is used for equal search
                      const std::map<std::string, range>& rn_attr,    //This map is used for range search
                      std::vector<std::map<std::string, std::string> >* sresults,
                      std::vector<std::map<std::string, uint64_t> >* nresults)
{      
    std::vector<hyperclient_range_query> rn; 
    std::vector<hyperclient_attribute> eqattr;

    for (std::map<std::string, val_type>::const_iterator ci = eq_attr.begin();
            ci != eq_attr.end(); ++ci)
    {
        hyperclient_attribute at;
        at.attr = ci->first.c_str();
        at.value = ci->second.first.c_str();
        at.value_sz = ci->second.first.size();
        at.datatype = ci->second.second;
        eqattr.push_back(at);
    }

    for (std::map<std::string, range>::const_iterator ci = rn_attr.begin();
            ci != rn_attr.end(); ++ci)
    {
        hyperclient_range_query rnattr;
        rnattr.attr = ci->first.c_str();
        rnattr.lower = ci->second.first;
        rnattr.upper = ci->second.second;
        rn.push_back(rnattr);
    }


    int64_t id;
    hyperclient_returncode status = HYPERCLIENT_A;
    hyperclient_attribute* attrs = NULL;
    size_t attrs_sz = 0;
    
    id = m_client.search(space.c_str(),
                         &eqattr.front(), eqattr.size(),
                         &rn.front(), rn.size(),
                         &status, &attrs, &attrs_sz);

    int64_t lid;
    hyperclient_returncode lstatus = HYPERCLIENT_B;
    size_t pre_insert_size = 0; // Keep track of the number of "n-tuples" returned
    while ((lid = m_client.loop(-1, &lstatus)) == id)
    {
        if (status == HYPERCLIENT_SEARCHDONE)
        {
            return HYPERCLIENT_SUCCESS;
        }
        if (status != HYPERCLIENT_SUCCESS)
        {
            break;
        }
        for (size_t i = 0; i < attrs_sz; ++i)
        {
            if ( attrs[i].datatype == HYPERDATATYPE_STRING )
            {
                if (sresults->empty() || sresults->size() == pre_insert_size)
                {
                    sresults->push_back(std::map<std::string, std::string>());
                }
                sresults->back().insert(std::make_pair(attrs[i].attr, std::string(attrs[i].value, attrs[i].value_sz)));
            }
            else if ( attrs[i].datatype == HYPERDATATYPE_INT64 )
            {
                if (nresults->empty() || nresults->size() == pre_insert_size)
                {
                    nresults->push_back(std::map<std::string, uint64_t>());
                }
                nresults->back().insert(std::make_pair(attrs[i].attr, le64toh(*((uint64_t *)(attrs[i].value)))));
            }
        }
        pre_insert_size++;
        if (attrs)
        {
            hyperclient_destroy_attrs(attrs, attrs_sz);
        }

        attrs = NULL;
        attrs_sz = 0;
    }

    if (lid < 0)
    {
        assert(static_cast<unsigned>(lstatus) >= 8448);
        assert(static_cast<unsigned>(lstatus) < 8576);
        return lstatus;
    }

    assert(lid == id);
    assert(static_cast<unsigned>(status) >= 8448);
    assert(static_cast<unsigned>(status) < 8576);
    return status;
 }

