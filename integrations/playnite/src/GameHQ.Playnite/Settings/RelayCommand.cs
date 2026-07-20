using System;
using System.Windows.Input;

namespace GameHQ.Playnite.Settings
{
    // Minimal ICommand for the settings page's buttons — every command here
    // is always enabled, so CanExecuteChanged is never raised.
    internal sealed class RelayCommand : ICommand
    {
        private readonly Action _execute;

        public RelayCommand(Action execute)
        {
            _execute = execute;
        }

        public event EventHandler CanExecuteChanged { add { } remove { } }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            _execute();
        }
    }
}
