import { useEffect, useState } from 'react'
import { NavLink } from 'react-router-dom'
import { LayoutDashboard, Settings, ShieldAlert, Wifi, WifiOff } from 'lucide-react'
import { cn } from '@/lib/utils'
import { Separator } from '@/components/ui/separator'

// Les pages disponibles dans la navigation
const NAV_ITEMS = [
  { to: '/',         label: 'Dashboard', icon: LayoutDashboard },
  { to: '/settings', label: 'Settings',  icon: Settings },
]

// Composant qui vérifie si l'API backend est accessible
function ApiStatus() {
  const [isOnline, setIsOnline] = useState(null) // null = vérification en cours

  useEffect(() => {
    // On fait un simple GET pour voir si le backend répond
    fetch('/api/machines')
      .then(() => setIsOnline(true))
      .catch(() => setIsOnline(false))
  }, [])

  return (
    <div className="flex items-center gap-2.5 rounded-lg bg-muted/40 px-3 py-2.5 ring-1 ring-border">
      {/* Icône selon l'état */}
      <div className={cn(
        'flex h-6 w-6 items-center justify-center rounded-md flex-shrink-0',
        isOnline === true  && 'bg-primary/10',
        isOnline === false && 'bg-destructive/10',
        isOnline === null  && 'bg-muted',
      )}>
        {isOnline === true  && <Wifi className="h-3.5 w-3.5 text-primary" />}
        {isOnline === false && <WifiOff className="h-3.5 w-3.5 text-destructive" />}
        {isOnline === null  && <Wifi className="h-3.5 w-3.5 text-muted-foreground animate-pulse" />}
      </div>

      {/* Texte de statut */}
      <div className="min-w-0">
        <p className="text-xs font-medium text-foreground">
          {isOnline === true  ? 'Backend Online'  : ''}
          {isOnline === false ? 'Backend Offline' : ''}
          {isOnline === null  ? 'Connexion...'    : ''}
        </p>
        <p className="text-[10px] text-muted-foreground">port 5000</p>
      </div>

      {/* Point vert animé si connecté */}
      {isOnline === true && (
        <span className="ml-auto h-1.5 w-1.5 rounded-full bg-primary animate-pulse flex-shrink-0" />
      )}
    </div>
  )
}

export default function Sidebar() {
  return (
    <aside className="flex h-screen w-60 flex-shrink-0 flex-col border-r border-border bg-card">

      {/* Logo / titre de l'app */}
      <div className="flex h-14 items-center gap-3 border-b border-border px-4">
        <div className="flex h-8 w-8 items-center justify-center rounded-lg bg-primary/10 ring-1 ring-primary/25">
          <ShieldAlert className="h-4 w-4 text-primary" />
        </div>
        <div className="flex items-baseline gap-2">
          <span className="text-sm font-semibold tracking-tight">SpyControl</span>
          <span className="rounded bg-primary/15 px-1.5 py-0.5 text-[10px] font-bold uppercase tracking-wider text-primary">
            C2
          </span>
        </div>
      </div>

      {/* Liens de navigation */}
      <nav className="flex flex-col gap-0.5 p-3 flex-1">
        <p className="px-2 pb-2 pt-1 text-[10px] font-semibold uppercase tracking-widest text-muted-foreground/60">
          Navigation
        </p>

        {NAV_ITEMS.map(({ to, label, icon: Icon }) => (
          <NavLink
            key={to}
            to={to}
            end
            className={({ isActive }) =>
              cn(
                'flex items-center gap-3 rounded-lg px-3 py-2 text-sm transition-colors',
                isActive
                  ? 'bg-primary/10 text-primary ring-1 ring-primary/20 font-medium'
                  : 'text-muted-foreground hover:bg-accent hover:text-accent-foreground'
              )
            }
          >
            <Icon className="h-4 w-4 flex-shrink-0" />
            {label}
          </NavLink>
        ))}
      </nav>

      {/* Statut de l'API en bas de la sidebar */}
      <div className="p-3 border-t border-border">
        <Separator className="mb-3" />
        <ApiStatus />
      </div>

    </aside>
  )
}
