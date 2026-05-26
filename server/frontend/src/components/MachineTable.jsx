import { useState } from 'react'
import { useNavigate } from 'react-router-dom'
import { Monitor, Trash2, Loader2 } from 'lucide-react'
import {
  Table, TableBody, TableCell, TableHead, TableHeader, TableRow,
} from '@/components/ui/table'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'
import { Skeleton } from '@/components/ui/skeleton'
import { cn } from '@/lib/utils'
import { deleteMachine } from '@/services/api'

function getStatus(lastSeen) {
  if (!lastSeen) return 'unknown'
  const diff = Date.now() - new Date(lastSeen).getTime()
  if (diff < 5 * 60 * 1000)  return 'online'
  if (diff < 30 * 60 * 1000) return 'idle'
  return 'offline'
}

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

function SkeletonRows() {
  return Array.from({ length: 5 }).map((_, i) => (
    <TableRow key={i} className="hover:bg-transparent">
      <TableCell><Skeleton className="h-4 w-8" /></TableCell>
      <TableCell><Skeleton className="h-2 w-2 rounded-full" /></TableCell>
      <TableCell><Skeleton className="h-4 w-36" /></TableCell>
      <TableCell><Skeleton className="h-5 w-24" /></TableCell>
      <TableCell><Skeleton className="h-4 w-28" /></TableCell>
      <TableCell><Skeleton className="h-4 w-28" /></TableCell>
      <TableCell><Skeleton className="h-6 w-6" /></TableCell>
    </TableRow>
  ))
}

export default function MachineTable({ machines, loading, onDelete }) {
  const navigate = useNavigate()
  const [deleting, setDeleting] = useState(new Set())

  async function handleDelete(e, id) {
    e.stopPropagation()
    setDeleting((prev) => new Set(prev).add(id))
    try {
      await deleteMachine(id)
      onDelete?.(id)
    } finally {
      setDeleting((prev) => { const s = new Set(prev); s.delete(id); return s })
    }
  }

  if (loading) {
    return (
      <Table>
        <TableHeader>
          <TableRow>
            <TableHead>#</TableHead><TableHead></TableHead>
            <TableHead>Hostname</TableHead><TableHead>OS</TableHead>
            <TableHead>IP Privée</TableHead><TableHead>IP Publique</TableHead>
            <TableHead className="w-10"></TableHead>
          </TableRow>
        </TableHeader>
        <TableBody><SkeletonRows /></TableBody>
      </Table>
    )
  }

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
          <TableHead className="w-10"></TableHead>
        </TableRow>
      </TableHeader>
      <TableBody>
        {machines.map((machine) => (
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
            <TableCell>
              <Button
                variant="ghost"
                size="icon"
                className="h-7 w-7 text-muted-foreground hover:text-destructive hover:bg-destructive/10"
                disabled={deleting.has(machine.id)}
                onClick={(e) => handleDelete(e, machine.id)}
              >
                {deleting.has(machine.id)
                  ? <Loader2 className="h-3.5 w-3.5 animate-spin" />
                  : <Trash2 className="h-3.5 w-3.5" />
                }
              </Button>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  )
}
