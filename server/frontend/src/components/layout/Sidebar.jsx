import { NavLink } from 'react-router-dom'
import { LayoutDashboard, Settings, ShieldAlert } from 'lucide-react'
import { cn } from '@/lib/utils'

const navItems = [
  { to: '/',         label: 'Dashboard', icon: LayoutDashboard },
  { to: '/settings', label: 'Settings',  icon: Settings },
]

export default function Sidebar() {
  return (
    <aside className="flex h-screen w-56 flex-shrink-0 flex-col border-r border-border bg-card">
      <div className="flex h-14 items-center gap-2 border-b border-border px-4">
        <ShieldAlert className="h-5 w-5 text-primary" />
        <span className="font-semibold tracking-tight text-foreground">SpyControl</span>
      </div>

      <nav className="flex flex-col gap-1 p-3">
        {navItems.map(({ to, label, icon: Icon }) => (
          <NavLink
            key={to}
            to={to}
            end
            className={({ isActive }) =>
              cn(
                'flex items-center gap-3 rounded-md px-3 py-2 text-sm transition-colors',
                isActive
                  ? 'bg-accent text-accent-foreground font-medium'
                  : 'text-muted-foreground hover:bg-accent hover:text-accent-foreground'
              )
            }
          >
            <Icon className="h-4 w-4" />
            {label}
          </NavLink>
        ))}
      </nav>
    </aside>
  )
}
