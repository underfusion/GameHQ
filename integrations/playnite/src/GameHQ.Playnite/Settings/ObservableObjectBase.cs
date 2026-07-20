using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace GameHQ.Playnite.Settings
{
    // Minimal INotifyPropertyChanged base. Playnite.SDK's own ISettings
    // contract doesn't require a specific base class, so this stays local
    // rather than guessing at an SDK-provided one.
    public abstract class ObservableObjectBase : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        protected void SetValue<T>(ref T field, T value, [CallerMemberName] string propertyName = null)
        {
            if (EqualityComparer<T>.Default.Equals(field, value)) return;
            field = value;
            OnPropertyChanged(propertyName);
        }

        protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            var handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
