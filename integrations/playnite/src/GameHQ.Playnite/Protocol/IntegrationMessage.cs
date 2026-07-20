using System;
using System.Collections.Generic;
using System.Text;
using System.Web.Script.Serialization;

namespace GameHQ.Playnite.Protocol
{
    // A single GameHQ.Local.v1 protocol message. Backed by a plain dictionary
    // so the client can speak the full message set from
    // docs/integration-protocol.md without a generated class per type.
    internal sealed class IntegrationMessage
    {
        private static readonly JavaScriptSerializer Serializer = new JavaScriptSerializer();

        private readonly Dictionary<string, object> _fields;

        public IntegrationMessage(string type)
        {
            if (string.IsNullOrEmpty(type))
                throw new ArgumentException("type is required", "type");

            _fields = new Dictionary<string, object> { { "type", type } };
        }

        private IntegrationMessage(Dictionary<string, object> fields)
        {
            _fields = fields;
        }

        public string Type
        {
            get { return GetString("type"); }
        }

        public IntegrationMessage Set(string key, object value)
        {
            _fields[key] = value;
            return this;
        }

        public string GetString(string key)
        {
            object value;
            return _fields.TryGetValue(key, out value) ? value as string : null;
        }

        public int? GetInt(string key)
        {
            object value;
            if (!_fields.TryGetValue(key, out value) || value == null) return null;
            try { return Convert.ToInt32(value); } catch (Exception) { return null; }
        }

        public bool TryGet(string key, out object value)
        {
            return _fields.TryGetValue(key, out value);
        }

        public byte[] ToUtf8Json()
        {
            var json = Serializer.Serialize(_fields);
            return Encoding.UTF8.GetBytes(json);
        }

        public static IntegrationMessage FromUtf8Json(byte[] payload)
        {
            var json = Encoding.UTF8.GetString(payload);
            var fields = Serializer.Deserialize<Dictionary<string, object>>(json);
            if (fields == null || !fields.ContainsKey("type"))
                throw new FormatException("Message is missing a type field");
            return new IntegrationMessage(fields);
        }
    }
}
