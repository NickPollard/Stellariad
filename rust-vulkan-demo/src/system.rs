

trait Env<E> {
    fn env(&self) -> &E;
}

impl<A,B> Env<A> for (A,B) {
    fn env(&self) -> &A {
        &self.0
    }
}

// TODO - can I do this like pluck?
/*
impl<A,B> Env<B> for (A,B) {
    fn env(&self) -> &B {
        &self.1
    }
}
*/

/*
trait HasEnv<S> {
    fn env_from(src: &S) -> &Self;
}

impl<E,S: Env<E>> HasEnv<S> for E {
    fn env_from(src: &S) -> &Self {
        S::env(src)
    }
}
*/

trait Sys {
    type Env;

    fn run(&self, env: &Self::Env);
}

trait SysEng<E> {
    fn run(&self, eng: &E);
}

impl<S, E> SysEng<E> for S
where S: Sys,
      E: Env<S::Env>
{
    fn run(&self, eng: &E) {
        self.run(E::env(eng))
    }
}

trait RunSysExt: Sized {
    fn run_sys<S: SysEng<Self>>(&self, system: S) {
        system.run(self);
    }
}
