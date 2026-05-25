import { useNavigate } from 'react-router-dom'
import { Monitor } from 'lucide-react'
import {
  Table, TableBody, TableCell, TableHead, TableHeader, TableRow,
} from '@/components/ui/table'
import { Badge } from '@/components/ui/badge'
import { Skeleton } from '@/components/ui/skeleton'
import { cn } from '@/lib/utils'

// Convertit une date ISO en temps relatif lisible ("2m ago", "1h ago"...)
function formatRelativeTime(dateStr) {
  if (!dateStr) return '—'
  const diff    = Date.now() - new Date(dateStr).getTime()
  const seconds = Math.floor(diff / 1000)
  if (seconds < 60)  return `${seconds}s ago`
  const minutes = Math.floor(seconds / 60)
  if (minutes < 60)  return `${minutes}m ago`
  const hours = Math.floor(minutes / 60)
  if (hours < 24)    return `${hours}h ago`
  return new Date(dateStr).toLocaleDateString()
}

// Détermine le statut de la machine selon quand elle a été vue pour la dernière fois
function getStatus(lastSeen) {
  if (!lastSeen) return 'unknown'
  const diff = Date.now() - new Date(lastSeen).getTime()
  if (diff < 5 * 60 * 1000)  return 'online'
  if (diff < 30 * 60 * 1000) return 'idle'
  return 'offline'
}

// Petit rond coloré indiquant le statut de la machine
function StatusDot({ lastSeen }) {
  const status = getStatus(lastSeen)
  return (
    <span className={cn('inline-block h-2 w-2 rounded-full', {
      'bg-primary animate-pulse': status === 'online',
      'bg-yellow-500':            status === 'idle',
      'bg-muted-foreground/40':   status === 'offline',
      'bg-muted-foreground/20':   status === 'unknown',
    })} />
  )
}

// Badge OS coloré selon le système d'exploitation détecté
function OSBadge({ osInfo }) {
  const raw   = osInfo ?? ''
  const lower = raw.toLowerCase()
  let label   = raw || 'Unknown'
  if (lower.includes('windows 11'))      label = 'Windows 11'
  else if (lower.includes('windows 10')) label = 'Windows 10'
  else if (lower.includes('windows'))    label = 'Windows'
  else if (lower.includes('linux'))      label = 'Linux'
  return (
    <Badge variant="outline" className={cn('text-xs font-mono', {
      'border-blue-500/25 bg-blue-500/10 text-blue-400':       lower.includes('windows'),
      'border-orange-500/25 bg-orange-500/10 text-orange-400': lower.includes('linux'),
    })}>
      {label}
    </Badge>
  )
}

// Affiche des lignes skeleton pendant que les données chargent
function SkeletonRows() {
  return Array.from({ length: 5 }).map((_, i) => (
    <TableRow key={i} className="hover:bg-transparent">
      <TableCell><Skeleton className="h-4 w-8" /></TableCell>
      <TableCell><Skeleton className="h-2 w-2 rounded-full" /></TableCell>
      <TableCell><Skeleton className="h-4 w-36" /></TableCell>
      <TableCell><Skeleton className="h-5 w-24" /></TableCell>
      <TableCell><Skeleton className="h-4 w-28" /></TableCell>
      <TableCell><Skeleton className="h-4 w-28" /></TableCell>
      <TableCell><Skeleton className="h-4 w-16" /></TableCell>
    </TableRow>
  ))
}

export default function MachineTable({ machines, loading }) {
  const navigate = useNavigate()

  // Pendant le chargement, on affiche le tableau avec des skeletons
  if (loading) {
    return (
      <Table>
        <TableHeader>
          <TableRow>
            <TableHead>#</TableHead><TableHead></TableHead>
            <TableHead>Hostname</TableHead><TableHead>OS</TableHead>
            <TableHead>IP Privée</TableHead><TableHead>IP Publique</TableHead>
            <TableHead>Vu il y a</TableHead>
          </TableRow>
        </TableHeader>
        <TableBody><SkeletonRows /></TableBody>
      </Table>
    )
  }

  // Si aucune machine n'a encore check-in
  if (machines.length === 0) {
    return (
      <div className="flex flex-col items-center gap-3 py-20 text-muted-foreground">
        <Monitor className="h-10 w-10 opacity-30" />
        <p className="text-sm font-medium">Aucun agent trouvé</p>
        <p className="text-xs opacity-60">En attente d'un check-in...</p>
      </div>
    )
  }

  return (
    <Table>
      <TableHeader>
        <TableRow>
          <TableHead className="w-10">#</TableHead>
          <TableHead className="w-8"></TableHead>
          <TableHead>Hostname</TableHead>
          <TableHead>OS</TableHead>
          <TableHead>IP Privée</TableHead>
          <TableHead>IP Publique</TableHead>
          <TableHead>Vu il y a</TableHead>
        </TableRow>
      </TableHeader>
      <TableBody>
        {machines.map((machine) => (
          // Chaque ligne est cliquable et navigue vers la page de détail
          <TableRow
            key={machine.id}
            className="cursor-pointer hover:bg-muted/50"
            onClick={() => navigate(`/machine/${machine.id}`)}
          >
            <TableCell className="text-xs text-muted-foreground/50">{machine.id}</TableCell>
            <TableCell><StatusDot lastSeen={machine.last_seen} /></TableCell>
            <TableCell className="font-medium">{machine.hostname}</TableCell>
            <TableCell><OSBadge osInfo={machine.os_info} /></TableCell>
            <TableCell className="font-mono text-xs text-muted-foreground">{machine.private_ip ?? '—'}</TableCell>
            <TableCell className="font-mono text-xs text-muted-foreground">{machine.public_ip ?? '—'}</TableCell>
            <TableCell className="text-xs text-muted-foreground">{formatRelativeTime(machine.last_seen)}</TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  )
}
